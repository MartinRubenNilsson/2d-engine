#include "stdafx.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_tiled.h"
#include "console.h"

#ifdef _DEBUG
#pragma comment(lib, "box2d-d.lib")
#else
#pragma comment(lib, "box2d.lib")
#endif

#ifdef _DEBUG_PHYSICS
#include "shapes.h"
#endif

namespace ecs {
	constexpr float _PHYSICS_LENGTH_UNITS_PER_METER = 16.f; // 16 pixels per meter
	constexpr float _PHYSICS_TIME_STEP = 1.f / 60.f;
	constexpr int _PHYSICS_SUB_STEP_COUNT = 4;

	b2WorldId _physics_world = b2_nullWorldId;
	float _physics_time_accumulator = 0.f;

	extern entt::registry _registry;

	void _on_destroy_b2BodyId(entt::registry& registry, entt::entity entity) {
		b2DestroyBody(registry.get<b2BodyId>(entity));
	}

	void startup_physics() {
		b2SetLengthUnitsPerMeter(_PHYSICS_LENGTH_UNITS_PER_METER);
		b2WorldDef world_def = b2DefaultWorldDef();
		world_def.gravity = { 0.f, 0.f }; // no gravity
		_physics_world = b2CreateWorld(&world_def);
		_registry.on_destroy<b2BodyId>().connect<_on_destroy_b2BodyId>();
	}

	void shutdown_physics() {
		_registry.on_destroy<b2BodyId>().disconnect<_on_destroy_b2BodyId>();
		b2DestroyWorld(_physics_world);
		_physics_world = b2_nullWorldId;
	}

	void setup_physics() {
		for (auto [entity, object] : _registry.view<ObjectId>().each()) {
			const Tag tag = get_tag(entity);

			switch (get_type(object)) {
				case ObjectType::Tile: {

					const TileId tile = get_tile(object);
					if (!tile) break;

					const std::span<const ObjectId> objects = get_objects(tile);
					if (!objects.empty()) {

						// DETERMINE PIVOT

						Vector2f pivot;

						for (ObjectId tile_object : objects) {
							if (get_type(tile_object) != ObjectType::Point)
								continue;
							if (get_name(tile_object) != "pivot")
								continue;
							pivot = get_position(tile_object);
						}

						// TODO!
						//sprite.sorting_point = pivot;

						// EMPLACE SPRITE-BODY ATTACHMENT

						// TODO!
						//make_sprite_follow_body(entity);

						// EMPLACE BODY

						b2BodyDef body_def = b2DefaultBodyDef();
						body_def.type = b2_dynamicBody;
						body_def.fixedRotation = true;
						body_def.position = get_top_left(object);
						b2BodyId body = emplace_body(entity, body_def);

						for (ObjectId collider : objects) {

							const Vector2f pos = get_position(collider);
							const Vector2f half_size = get_size(collider) * 0.5f;
							const Vector2f center = pos + half_size;

							switch (get_type(collider)) {
								case ObjectType::Rectangle: {

									b2ShapeDef shape_def = b2DefaultShapeDef();
									shape_def.filter = get_physics_filter_for_tag(tag);
									b2Polygon box = b2MakeOffsetBox(half_size.x, half_size.y, center, 0.f);
									b2CreatePolygonShape(body, &shape_def, &box);

								} break;
								case ObjectType::Ellipse: {

									b2ShapeDef shape_def = b2DefaultShapeDef();
									shape_def.filter = get_physics_filter_for_tag(tag);
									b2Circle circle{};
									circle.center = center;
									circle.radius = half_size.x;
									b2CreateCircleShape(body, &shape_def, &circle);

								} break;
							}
						}
					}
				} break;
				default: { // Rectangle, Ellipse, Point, Polygon, Polyline

					// CREATE SENSORS

					b2BodyDef body_def = b2DefaultBodyDef();
					body_def.type = b2_staticBody;
					body_def.fixedRotation = true;
					body_def.position = get_top_left(object);
					b2BodyId body = emplace_body(entity, body_def);

					const Vector2f half_size = get_position(object);
					const Vector2f center = half_size;

					switch (get_type(object)) {
						case ObjectType::Rectangle: {

							b2ShapeDef shape_def = b2DefaultShapeDef();
							shape_def.isSensor = true;
							shape_def.filter = get_physics_filter_for_tag(tag);
							b2Polygon box = b2MakeOffsetBox(half_size.x, half_size.y, center, 0.f);
							b2CreatePolygonShape(body, &shape_def, &box);

						} break;
						case ObjectType::Ellipse: {

							b2ShapeDef shape_def = b2DefaultShapeDef();
							shape_def.isSensor = true;
							shape_def.filter = get_physics_filter_for_tag(tag);
							b2Circle circle{};
							circle.center = center;
							circle.radius = half_size.x;
							b2CreateCircleShape(body, &shape_def, &circle);

						} break;
					}

				} break;
			}
		}

		for (auto [entity, tile, tile_pos] : _registry.view<TileId, Vector2u>().each()) {
			if (!tile)
				continue;

			const std::span<const ObjectId> colliders = get_objects(tile);
			if (colliders.empty())
				continue;

			const Vector2f position = {
				tile_pos.x * _PHYSICS_LENGTH_UNITS_PER_METER,
				tile_pos.y * _PHYSICS_LENGTH_UNITS_PER_METER
			};

			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_staticBody;
			body_def.position = position;
			body_def.fixedRotation = true;
			b2BodyId body = emplace_body(entity, body_def);

			for (const ObjectId collider : colliders) {
				const Vector2f center = get_position(collider);
				const Vector2f half_size = get_size(collider) * 0.5f;

				b2ShapeDef shape_def = b2DefaultShapeDef();
				if (get_bool(collider, "sensor")) {
					shape_def.isSensor = true;
				}

				switch (get_type(collider)) {
					case ObjectType::Rectangle: {

						b2Polygon box = b2MakeOffsetBox(
							half_size.x,
							half_size.y,
							center + half_size, 0.f);
						b2CreatePolygonShape(body, &shape_def, &box);

					} break;
					case ObjectType::Ellipse: {

						b2Circle circle{};
						circle.center = center;
						circle.radius = half_size.x;
						b2CreateCircleShape(body, &shape_def, &circle);

					} break;
					case ObjectType::Polygon: {

						const std::span<const Vector2f> points = get_points(collider);
						const int32_t count = (int32_t)points.size();
						if (count < 3) {
							console::log_error("Too few points in polygon collider! Got " + std::to_string(count) + ", need >= 3.");
							break;
						}

						if (count <= b2_maxPolygonVertices && is_convex(points)) {

							b2Vec2 polygon_points[b2_maxPolygonVertices];
							for (int32_t i = 0; i < count; ++i) {
								polygon_points[i] = center + points[i];
							}
							b2Hull hull = b2ComputeHull(polygon_points, count);
							if (!b2ValidateHull(&hull)) {
								console::log_error("Invalid hull in polygon collider!");
								break;
							}
							b2Polygon polygon = b2MakePolygon(&hull, 0.f);
							b2CreatePolygonShape(body, &shape_def, &polygon);
							break;
						}

						//TODO: fix triangulate()
						const std::vector<Vector2f> triangles = triangulate(points);
						for (size_t i = 0; i < triangles.size(); i += 3) {
							b2Vec2 triangle_points[3];
							for (size_t j = 0; j < 3; ++j) {
								triangle_points[j] = center + triangles[i + j];
							}
							b2Hull hull = b2ComputeHull(triangle_points, 3);
							if (!b2ValidateHull(&hull)) {
								console::log_error("Invalid hull in polygon collider!");
								continue;
							}
							b2Polygon polygon = b2MakePolygon(&hull, 0.f);
							b2CreatePolygonShape(body, &shape_def, &polygon);
						}

					} break;
					/*case tiled::ObjectType::Point: {

						sprite.sorting_point = Vector2f(collider.x, collider.y);

					} break;*/
				}
			}
		}
	}

	void update_physics(float dt) {
		_physics_time_accumulator += dt;
		for (; _physics_time_accumulator >= _PHYSICS_TIME_STEP; _physics_time_accumulator -= _PHYSICS_TIME_STEP) {

			// STEP PHYSICS WORLD

			b2World_Step(_physics_world, _PHYSICS_TIME_STEP, _PHYSICS_SUB_STEP_COUNT);

			// PROCESS SENSOR EVENTS
			{
				const b2SensorEvents sensor_events = b2World_GetSensorEvents(_physics_world);
				for (int32_t i = 0; i < sensor_events.beginCount; ++i) {
					const b2SensorBeginTouchEvent& b2_ev = sensor_events.beginEvents[i];
					PhysicsEvent ev{};
					ev.type = PhysicsEventType::SensorBeginTouch;
					ev.shape_a = b2_ev.sensorShapeId;
					ev.shape_b = b2_ev.visitorShapeId;
					ev.body_a = b2Shape_GetBody(b2_ev.sensorShapeId);
					ev.body_b = b2Shape_GetBody(b2_ev.visitorShapeId);
					ev.entity_a = (entt::entity)(uintptr_t)b2Body_GetUserData(ev.body_a);
					ev.entity_b = (entt::entity)(uintptr_t)b2Body_GetUserData(ev.body_b);
					ev.tag_a = get_tag(ev.entity_a);
					ev.tag_b = get_tag(ev.entity_b);
					if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity_a)) {
						handler(ev);
					}
					std::swap(ev.shape_a, ev.shape_b);
					std::swap(ev.body_a, ev.body_b);
					std::swap(ev.entity_a, ev.entity_b);
					std::swap(ev.tag_a, ev.tag_b);
					if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity_a)) { // SIC: ev.entity_a since we swapped
						handler(ev);
					}
				}
				for (int32_t i = 0; i < sensor_events.endCount; ++i) {
					const b2SensorEndTouchEvent& b2_ev = sensor_events.endEvents[i];
					PhysicsEvent ev{};
					ev.type = PhysicsEventType::SensorEndTouch;
					ev.shape_a = b2_ev.sensorShapeId;
					ev.shape_b = b2_ev.visitorShapeId;
					ev.body_a = b2Shape_GetBody(b2_ev.sensorShapeId);
					ev.body_b = b2Shape_GetBody(b2_ev.visitorShapeId);
					ev.entity_a = (entt::entity)(uintptr_t)b2Body_GetUserData(ev.body_a);
					ev.entity_b = (entt::entity)(uintptr_t)b2Body_GetUserData(ev.body_b);
					ev.tag_a = get_tag(ev.entity_a);
					ev.tag_b = get_tag(ev.entity_b);
					if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity_a)) {
						handler(ev);
					}
					std::swap(ev.shape_a, ev.shape_b);
					std::swap(ev.body_a, ev.body_b);
					std::swap(ev.entity_a, ev.entity_b);
					std::swap(ev.tag_a, ev.tag_b);
					if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity_a)) { // SIC: ev.entity_a since we swapped
						handler(ev);
					}
				}
			}

			// PROCESS CONTACT EVENTS
			{
				const b2ContactEvents contact_events = b2World_GetContactEvents(_physics_world);
				for (int32_t i = 0; i < contact_events.beginCount; ++i) {
					const b2ContactBeginTouchEvent& b2_ev = contact_events.beginEvents[i];
					PhysicsEvent ev{};
					ev.type = PhysicsEventType::ContactBeginTouch;
					ev.shape_a = b2_ev.shapeIdA;
					ev.shape_b = b2_ev.shapeIdB;
					ev.body_a = b2Shape_GetBody(b2_ev.shapeIdA);
					ev.body_b = b2Shape_GetBody(b2_ev.shapeIdB);
					ev.entity_a = (entt::entity)(uintptr_t)b2Body_GetUserData(ev.body_a);
					ev.entity_b = (entt::entity)(uintptr_t)b2Body_GetUserData(ev.body_b);
					ev.tag_a = get_tag(ev.entity_a);
					ev.tag_b = get_tag(ev.entity_b);
					if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity_a)) {
						handler(ev);
					}
					if (B2_ID_EQUALS(b2_ev.shapeIdA, b2_ev.shapeIdB)) continue; // PITFALL: Avoid duplicate calls
					std::swap(ev.shape_a, ev.shape_b);
					std::swap(ev.body_a, ev.body_b);
					std::swap(ev.entity_a, ev.entity_b);
					std::swap(ev.tag_a, ev.tag_b);
					if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity_a)) { // SIC: ev.entity_a since we swapped
						handler(ev);
					}
				}
				for (int32_t i = 0; i < contact_events.endCount; ++i) {
					const b2ContactEndTouchEvent& b2_ev = contact_events.endEvents[i];
					PhysicsEvent ev{};
					ev.type = PhysicsEventType::ContactEndTouch;
					ev.shape_a = b2_ev.shapeIdA;
					ev.shape_b = b2_ev.shapeIdB;
					ev.body_a = b2Shape_GetBody(b2_ev.shapeIdA);
					ev.body_b = b2Shape_GetBody(b2_ev.shapeIdB);
					ev.entity_a = (entt::entity)(uintptr_t)b2Body_GetUserData(ev.body_a);
					ev.entity_b = (entt::entity)(uintptr_t)b2Body_GetUserData(ev.body_b);
					ev.tag_a = get_tag(ev.entity_a);
					ev.tag_b = get_tag(ev.entity_b);
					if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity_a)) {
						handler(ev);
					}
					if (B2_ID_EQUALS(b2_ev.shapeIdA, b2_ev.shapeIdB)) continue; // PITFALL: Avoid duplicate calls
					std::swap(ev.shape_a, ev.shape_b);
					std::swap(ev.body_a, ev.body_b);
					std::swap(ev.entity_a, ev.entity_b);
					std::swap(ev.tag_a, ev.tag_b);
					if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity_a)) { // SIC: ev.entity_a since we swapped
						handler(ev);
					}
				}
			}
		}
	}

#ifdef _DEBUG_PHYSICS
	Color _b2HexColor_to_Color(b2HexColor hex_color) {
		Color color{};
		color.r = (hex_color >> 24) & 0xFF;
		color.g = (hex_color >> 16) & 0xFF;
		color.b = (hex_color >> 8) & 0xFF;
		color.a = 255;
		return color;
	}

	void _b2_debug_draw_polygon(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context) {
		shapes::add_polygon((const Vector2f*)vertices, vertexCount, _b2HexColor_to_Color(color));
	}

	void _b2_debug_draw_solid_polygon(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context) {
		b2Vec2 transformed_vertices[b2_maxPolygonVertices];
		for (int i = 0; i < vertexCount; ++i) {
			transformed_vertices[i] = b2TransformPoint(transform, vertices[i]);
		}
		shapes::add_polygon((const Vector2f*)transformed_vertices, vertexCount, _b2HexColor_to_Color(color));
	}

	void _b2_debug_draw_circle(b2Vec2 center, float radius, b2HexColor color, void* context) {
		shapes::add_circle(center, radius, _b2HexColor_to_Color(color));
	}

	void _b2_debug_draw_solid_circle(b2Transform transform, float radius, b2HexColor color, void* context) {
		shapes::add_circle(transform.p, radius, _b2HexColor_to_Color(color));
	}

	void _b2_debug_draw_capsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context) {
		//TODO
	}

	void _b2_debug_draw_segment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context) {
		shapes::add_line(p1, p2, _b2HexColor_to_Color(color));
	}

	void _b2_debug_draw_transform(b2Transform transform, void* context) {
		//TODO
	}

	void _b2_debug_draw_point(b2Vec2 p, float size, b2HexColor color, void* context) {
		//TODO
	}

	void _b2_debug_draw_string(b2Vec2 p, const char* s, void* context) {
		//TODO
	}
#endif

	void debug_draw_physics() {
#ifdef _DEBUG_PHYSICS
		b2DebugDraw debug_draw{};
		debug_draw.DrawPolygon = _b2_debug_draw_polygon;
		debug_draw.DrawSolidPolygon = _b2_debug_draw_solid_polygon;
		debug_draw.DrawCircle = _b2_debug_draw_circle;
		debug_draw.DrawSolidCircle = _b2_debug_draw_solid_circle;
		debug_draw.DrawCapsule = _b2_debug_draw_capsule;
		debug_draw.DrawSegment = _b2_debug_draw_segment;
		debug_draw.DrawTransform = _b2_debug_draw_transform;
		debug_draw.DrawPoint = _b2_debug_draw_point;
		debug_draw.DrawString = _b2_debug_draw_string;
		debug_draw.drawShapes = true;
		debug_draw.drawContacts = true;
		debug_draw.drawContactNormals = true;
		b2World_Draw(_physics_world, &debug_draw);
#endif
	}

	bool raycast_closest(const Vector2f& ray_start, const Vector2f& ray_end, uint32_t mask_bits, RaycastHit* hit) {
		b2QueryFilter query_filter = b2DefaultQueryFilter();
		query_filter.maskBits = mask_bits;

		const b2RayResult result = b2World_CastRayClosest(_physics_world, ray_start, ray_end - ray_start, query_filter);
		if (result.hit && hit) {
			hit->shape = result.shapeId;
			hit->body = b2Shape_GetBody(hit->shape);
			hit->entity = (entt::entity)(uintptr_t)b2Body_GetUserData(hit->body);
			hit->point = result.point;
			hit->normal = result.normal;
			hit->fraction = result.fraction;
		}

		return result.hit;
	}

	std::vector<RaycastHit> raycast(const Vector2f& ray_start, const Vector2f& ray_end, uint32_t mask_bits) {
		b2QueryFilter query_filter = b2DefaultQueryFilter();
		query_filter.maskBits = mask_bits;

		std::vector<RaycastHit> hits;

		b2World_CastRay(_physics_world, ray_start, ray_end - ray_start, query_filter,
			[](b2ShapeId shape_id, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
			RaycastHit hit{};
			hit.shape = shape_id;
			hit.body = b2Shape_GetBody(shape_id);
			hit.entity = (entt::entity)(uintptr_t)b2Body_GetUserData(hit.body);
			hit.point = point;
			hit.normal = normal;
			hit.fraction = fraction;
			((std::vector<RaycastHit>*)context)->push_back(hit);
			return 1.f;
		}, &hits);

		return hits;
	}

	std::vector<OverlapHit> overlap_box(const Vector2f& box_min, const Vector2f& box_max, uint32_t mask_bits) {
		const Vector2f box_half_size = 0.5 * (box_max - box_min);
		const Vector2f box_center = 0.5 * (box_min + box_max);
		b2Polygon box = b2MakeOffsetBox(box_half_size.x, box_half_size.y, box_center, 0.f);

		b2QueryFilter query_filter = b2DefaultQueryFilter();
		query_filter.maskBits = mask_bits;

		std::vector<OverlapHit> hits;
		b2World_OverlapPolygon(_physics_world, &box, b2Transform_identity, query_filter,
			[](b2ShapeId shape_id, void* context) {
			OverlapHit hit{};
			hit.shape = shape_id;
			hit.body = b2Shape_GetBody(shape_id);
			hit.entity = (entt::entity)(uintptr_t)b2Body_GetUserData(hit.body);
			((std::vector<OverlapHit>*)context)->push_back(hit);
			return true;
		}, &hits);

		return hits;
	}

	std::vector<OverlapHit> overlap_circle(const Vector2f& center, float radius, uint32_t mask_bits) {
		b2Circle circle{};
		circle.center = center;
		circle.radius = radius;

		b2QueryFilter query_filter = b2DefaultQueryFilter();
		query_filter.maskBits = mask_bits;

		std::vector<OverlapHit> hits;
		b2World_OverlapCircle(_physics_world, &circle, b2Transform_identity, query_filter,
			[](b2ShapeId shape_id, void* context) {
			OverlapHit hit{};
			hit.shape = shape_id;
			hit.body = b2Shape_GetBody(shape_id);
			hit.entity = (entt::entity)(uintptr_t)b2Body_GetUserData(hit.body);
			((std::vector<OverlapHit>*)context)->push_back(hit);
			return true;
		}, &hits);

		return hits;
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

	void for_each_shape(b2BodyId body, void(*func)(b2ShapeId shape)) {
		b2ShapeId shapes[16]; // assume body has at most 16 shapes
		int shape_count = b2Body_GetShapes(body, shapes, 16);
		for (int i = 0; i < shape_count; ++i) {
			func(shapes[i]);
		}
	}

	void set_category_bits(b2BodyId body, uint32_t category_bits) {
#if 0
		for (b2Fixture* fixture = b2Body_GetFixtureList(); fixture; fixture = fixture->GetNext()) {
			b2Filter filter = fixture->GetFilterData();
			filter.categoryBits = category_bits;
			fixture->SetFilterData(filter);
		}
#endif
	}

	uint32_t get_category_bits(b2BodyId body) {
		uint32_t category_bits = 0;
#if 0
		for (const b2Fixture* fixture = b2Body_GetFixtureList(); fixture; fixture = fixture->GetNext()) {
			category_bits |= fixture->GetFilterData().categoryBits;
		}
#endif
		return category_bits;
	}

	b2BodyId emplace_body(entt::entity entity, const b2BodyDef& body_def) {
		b2BodyDef body_def_copy = body_def;
		body_def_copy.userData = (void*)(uintptr_t)entity;
		b2BodyId body = b2CreateBody(_physics_world, &body_def_copy);
		_registry.emplace_or_replace<b2BodyId>(entity, body);
		return body;
	}


	b2BodyId deep_copy_and_emplace_body(entt::entity entity, b2BodyId body) {
		b2BodyDef body_def = get_body_def(body);
		b2BodyId new_body = b2CreateBody(_physics_world, &body_def);
#if 0
		for (const b2Fixture* fixture = b2Body_GetFixtureList(); fixture; fixture = fixture->GetNext()) {
			b2FixtureDef fixture_def = get_shape_def(fixture);
			new_body->CreateFixture(&fixture_def);
		}
#endif
		//HACK: so they don't spawn inside each other
		b2Vec2 pos = b2Body_GetPosition(body);
		pos.x += 16.f; //one tile
		b2Body_SetTransform(new_body, pos, { 0.f, 0.f });
		return _registry.emplace_or_replace<b2BodyId>(entity, new_body);
	}

	b2BodyId get_body(entt::entity entity) {
		b2BodyId* body_ptr = _registry.try_get<b2BodyId>(entity);
		return body_ptr ? *body_ptr : b2_nullBodyId;
	}

	bool remove_body(entt::entity entity) {
		return _registry.remove<b2BodyId>(entity);
	}

	void set_physics_event_handler(entt::entity entity, PhysicsEventHandler handler) {
		if (handler) {
			_registry.emplace_or_replace<PhysicsEventHandler>(entity, handler);
		} else {
			_registry.remove<PhysicsEventHandler>(entity);
		}
	}

	PhysicsEventHandler get_physics_event_handler(entt::entity entity) {
		PhysicsEventHandler* handler_ptr = _registry.try_get<PhysicsEventHandler>(entity);
		return handler_ptr ? *handler_ptr : nullptr;
	}
}
