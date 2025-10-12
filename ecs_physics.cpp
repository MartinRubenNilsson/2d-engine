#include "stdafx.h"
#include "ecs_physics.h"
#include "ecs_physics_events.h"
#include "ecs_cloning.h"

#ifdef _DEBUG
#pragma comment(lib, "box2d-d.lib")
#else
#pragma comment(lib, "box2d.lib")
#endif

namespace ecs {
	void swap(b2Manifold& manifold) {
		manifold.normal = -manifold.normal;
		for (int i = 0; i < manifold.pointCount; ++i) {
			b2ManifoldPoint& point = manifold.points[i];
			std::swap(point.anchorA, point.anchorB);
			// TODO: should we invert or swap anything else?
		}
	}

	void swap(b2ContactData& contact) {
		std::swap(contact.shapeIdA, contact.shapeIdB);
		swap(contact.manifold);
	}

	void _on_destroy_b2BodyId(entt::registry& registry, entt::entity entity) {
		b2DestroyBody(registry.get<b2BodyId>(entity));
	}

	extern entt::registry _registry;

	void _clone_b2BodyId(entt::entity clone, const void* component) {
		b2BodyId body = *(b2BodyId*)component;
		b2BodyDef body_def = get_def(body);
		b2BodyId body_clone = emplace_body(clone, body_def);
#if 0
		for (const b2Fixture* fixture = b2Body_GetFixtureList(); fixture; fixture = fixture->GetNext()) {
			b2FixtureDef fixture_def = get_shape_def(fixture);
			new_body->CreateFixture(&fixture_def);
		}
#endif
		//HACK: so they don't spawn inside each other
		b2Vec2 pos = b2Body_GetPosition(body);
		pos.x += 16.f; //one tile
		b2Body_SetTransform(body_clone, pos, { 0.f, 0.f });
	}
	
	b2WorldId _physics_world = b2_nullWorldId;

	void startup_physics() {
		constexpr float LENGTH_UNITS_PER_METER = 16.f; // 16 pixels per meter
		b2SetLengthUnitsPerMeter(LENGTH_UNITS_PER_METER);
		b2WorldDef def = b2DefaultWorldDef();
		def.gravity = Vec2f::ZERO; // no gravity
		_physics_world = b2CreateWorld(&def);
		_registry.on_destroy<b2BodyId>().connect<_on_destroy_b2BodyId>();
		set_cloning_handler(entt::type_id<b2BodyId>(), _clone_b2BodyId);
	}

	void shutdown_physics() {
		_registry.on_destroy<b2BodyId>().disconnect<_on_destroy_b2BodyId>();
		b2DestroyWorld(_physics_world);
		_physics_world = b2_nullWorldId;
	}

	void clear_physics() {
		// Do a zero-time update to force a refresh of the physics world.
		// Internally this just clears all the physics event buffers.
		b2World_Step(_physics_world, 0.f, 1);
	}

	void update_physics(float dt) {
		clear_physics_events();
		constexpr float TIME_STEP = 1.f / 60.f;
		constexpr int SUB_STEP_COUNT = 4;
		static float time_accumulator = 0.f;
		time_accumulator += dt;
		for (; time_accumulator >= TIME_STEP; time_accumulator -= TIME_STEP) {
			b2World_Step(_physics_world, TIME_STEP, SUB_STEP_COUNT);
			dispatch_physics_events();
		}
	}

	void _set_entity(b2BodyId body, entt::entity entity) {
		b2Body_SetUserData(body, (void*)(uintptr_t)entity);
	}

	b2BodyId emplace_body(entt::entity entity, const b2BodyDef& def) {
		b2BodyId body = b2CreateBody(_physics_world, &def);
		_registry.emplace_or_replace<b2BodyId>(entity, body);
		_set_entity(body, entity);
		return body;
	}

	b2BodyId get_body(entt::entity entity) {
		b2BodyId* body_ptr = _registry.try_get<b2BodyId>(entity);
		return body_ptr ? *body_ptr : b2_nullBodyId;
	}

	entt::entity get_entity(b2BodyId body) {
		void* user_data = b2Body_GetUserData(body);
		if (!user_data) return entt::null;
		return (entt::entity)(uintptr_t)user_data;
	}

	b2BodyDef get_def(b2BodyId body) {
		b2BodyDef def = b2DefaultBodyDef();
		def.type = b2Body_GetType(body);
		def.position = b2Body_GetPosition(body);
		def.rotation = b2Body_GetRotation(body);
		def.linearVelocity = b2Body_GetLinearVelocity(body);
		def.angularVelocity = b2Body_GetAngularVelocity(body);
		def.linearDamping = b2Body_GetLinearDamping(body);
		def.angularDamping = b2Body_GetAngularDamping(body);
		def.gravityScale = b2Body_GetGravityScale(body);
		def.sleepThreshold = b2Body_GetSleepThreshold(body);
		def.userData = b2Body_GetUserData(body);
		def.enableSleep = b2Body_IsSleepEnabled(body);
		def.isAwake = b2Body_IsAwake(body);
		def.fixedRotation = b2Body_IsFixedRotation(body);
		def.isBullet = b2Body_IsBullet(body);
		def.isEnabled = b2Body_IsEnabled(body);
		//def.automaticMass = ???
		//def.allowFastRotation = ???
		return def;
	}

	std::span<const b2ShapeId> get_shapes(b2BodyId body) {
		constexpr size_t CAPACITY = 32;
		static b2ShapeId shapes[CAPACITY];
		size_t count = b2Body_GetShapeCount(body);
		assert(count < CAPACITY);
		count = b2Body_GetShapes(body, shapes, CAPACITY);
		return { shapes, count };
	}

	// This helper function gives no guarantees on whether shapeIdA or shapeIdB is
	// the shape that belongs to the body. (This is just how the Box2D API works.)
	std::span<b2ContactData> _get_unordered_contacts(b2BodyId body) {
		constexpr size_t CAPACITY = 32;
		static b2ContactData contacts[CAPACITY];
		size_t count = b2Body_GetContactCapacity(body);
		assert(count < CAPACITY);
		count = b2Body_GetContactData(body, contacts, CAPACITY);
		return { contacts, count };
	}

	std::span<const b2ContactData> get_contacts(b2BodyId body) {
		const std::span<b2ContactData> contacts = _get_unordered_contacts(body);
		if (contacts.empty()) return {};
		// For each contact, if shapeIdB belongs to the body, swap the contact.
		const std::span<const b2ShapeId> shapes = get_shapes(body);
		for (b2ContactData& contact : contacts) {
			for (const b2ShapeId& shape : shapes) {
				if (B2_ID_EQUALS(contact.shapeIdB, shape)) {
					swap(contact);
					break; // Move on to the next contact.
				}
			}
		}
		return contacts;
	}

	uint32_t get_category_bits(b2BodyId body) {
		uint32_t category_bits = 0;
		for (b2ShapeId shape : get_shapes(body)) {
			category_bits |= b2Shape_GetFilter(shape).categoryBits;
		}
		return category_bits;
	}

	void remove_body(entt::entity entity) {
		_registry.remove<b2BodyId>(entity);
	}

	void stop_moving(entt::entity entity) {
		b2BodyId* body = _registry.try_get<b2BodyId>(entity);
		if (!body) return;
		b2Body_SetLinearVelocity(*body, Vec2f::ZERO);
	}
}
