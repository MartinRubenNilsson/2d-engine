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
	void _on_destroy_b2BodyId(entt::registry& registry, entt::entity entity) {
		b2DestroyBody(registry.get<b2BodyId>(entity));
	}

	extern entt::registry _registry;

	void _clone_b2BodyId(entt::entity clone, const void* component) {
		b2BodyId body = *(b2BodyId*)component;
		b2BodyDef body_def = get_body_def(body);
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
		b2WorldDef world_def = b2DefaultWorldDef();
		world_def.gravity = { 0.f, 0.f }; // no gravity
		_physics_world = b2CreateWorld(&world_def);
		_registry.on_destroy<b2BodyId>().connect<_on_destroy_b2BodyId>();
		set_cloning_handler(entt::type_id<b2BodyId>(), _clone_b2BodyId);
	}

	void shutdown_physics() {
		_registry.on_destroy<b2BodyId>().disconnect<_on_destroy_b2BodyId>();
		b2DestroyWorld(_physics_world);
		_physics_world = b2_nullWorldId;
	}

	void update_physics(float dt) {
		constexpr float TIME_STEP = 1.f / 60.f;
		constexpr int SUB_STEP_COUNT = 4;
		static float time_accumulator = 0.f;
		time_accumulator += dt;
		for (; time_accumulator >= TIME_STEP; time_accumulator -= TIME_STEP) {
			b2World_Step(_physics_world, TIME_STEP, SUB_STEP_COUNT);
			// Dispatch sensor events.
			{
				const b2SensorEvents events = b2World_GetSensorEvents(_physics_world);
				for (int32_t i = 0; i < events.beginCount; ++i) {
					dispatch_physics_event(events.beginEvents[i]);
				}
				for (int32_t i = 0; i < events.endCount; ++i) {
					dispatch_physics_event(events.endEvents[i]);
				}
			}
			// Dispatch contact events.
			{
				const b2ContactEvents events = b2World_GetContactEvents(_physics_world);
				for (int32_t i = 0; i < events.beginCount; ++i) {
					dispatch_physics_event(events.beginEvents[i]);
				}
				for (int32_t i = 0; i < events.endCount; ++i) {
					dispatch_physics_event(events.endEvents[i]);
				}
			}
		}
	}

	b2ShapeDef get_shape_def(b2ShapeId shape) {
		b2ShapeDef def = b2DefaultShapeDef();
#if 0
		def.shape = fixture->GetShape();
		def.userData = fixture->GetUserData();
		def.friction = fixture->GetFriction();
		def.restitution = fixture->GetRestitution();
		def.restitutionThreshold = fixture->GetRestitutionThreshold();
		def.density = fixture->GetDensity();
		def.isSensor = fixture->IsSensor();
		def.filter = fixture->GetFilterData();
#endif
		return def;
	}

	b2BodyDef get_body_def(b2BodyId body) {
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

	std::span<const b2ShapeId> _get_shapes(b2BodyId body) {
		constexpr size_t CAPACITY = 16;
		b2ShapeId shapes[CAPACITY];
		const size_t num_shapes = b2Body_GetShapeCount(body);
		assert(num_shapes < CAPACITY);
		b2Body_GetShapes(body, shapes, CAPACITY);
		return { shapes, num_shapes };
	}

	uint32_t get_category_bits(b2BodyId body) {
		uint32_t category_bits = 0;
		for (b2ShapeId shape : _get_shapes(body)) {
			category_bits |= b2Shape_GetFilter(shape).categoryBits;
		}
		return category_bits;
	}

	b2BodyId emplace_body(entt::entity entity, const b2BodyDef& body_def) {
		b2BodyDef body_def_copy = body_def;
		body_def_copy.userData = (void*)(uintptr_t)entity;
		b2BodyId body = b2CreateBody(_physics_world, &body_def_copy);
		_registry.emplace_or_replace<b2BodyId>(entity, body);
		return body;
	}

	b2BodyId get_body(entt::entity entity) {
		b2BodyId* body_ptr = _registry.try_get<b2BodyId>(entity);
		return body_ptr ? *body_ptr : b2_nullBodyId;
	}

	bool remove_body(entt::entity entity) {
		return _registry.remove<b2BodyId>(entity);
	}
}
