#include "stdafx.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_sprites.h"
#include "console.h"

namespace ecs {
	extern entt::registry _registry;

	void _create_shapes(b2BodyId body, const b2ShapeDef& def, ObjectId object) {
		// PITFALL: Calling get_position() for non-tile objects returns the top left,
		// but for tile objects it returns the bottom left! Hence we call get_top_left()
		// here to make sure we're consistent.
		const Vec2f top_left = get_top_left(object);
		const Vec2f half_size = get_size(object) * 0.5f;
		const Vec2f center = top_left + half_size;

		switch (get_type(object)) {
			case ObjectType::Rectangle: {

				b2Polygon box = b2MakeOffsetBox(half_size.x, half_size.y, center, 0.f);
				b2CreatePolygonShape(body, &def, &box);

			} break;
			case ObjectType::Ellipse: {

				b2Circle circle{};
				circle.center = center;
				circle.radius = half_size.x; // Box2D doesn't support ellipses, so this is the best we can do.
				b2CreateCircleShape(body, &def, &circle);

			} break;
			case ObjectType::Point: {

				// This case is not supported.

			} break;
			case ObjectType::Polygon: {

				const std::span<const Vec2f> points = get_points(object);
				const int32_t num_points = (int32_t)points.size();
				if (num_points < 3) {
					console::log_error("Too few points in polygon collider! Got " + std::to_string(num_points) + ", need >= 3.");
					break;
				}

				if (num_points <= b2_maxPolygonVertices && convex(points)) {

					b2Vec2 polygon_points[b2_maxPolygonVertices];
					for (int32_t i = 0; i < num_points; ++i) {
						polygon_points[i] = top_left + points[i];
					}
					b2Hull hull = b2ComputeHull(polygon_points, num_points);
					if (!b2ValidateHull(&hull)) {
						console::log_error("Invalid hull in polygon collider!");
						break;
					}
					b2Polygon polygon = b2MakePolygon(&hull, 0.f);
					b2CreatePolygonShape(body, &def, &polygon);
					break;
				}

				//TODO: fix triangulate()! it has broken math.
				const std::vector<Vec2f> triangles = triangulate(points);
				for (size_t i = 0; i < triangles.size(); i += 3) {
					b2Vec2 triangle_points[3];
					for (size_t j = 0; j < 3; ++j) {
						triangle_points[j] = top_left + triangles[i + j];
					}
					b2Hull hull = b2ComputeHull(triangle_points, 3);
					if (!b2ValidateHull(&hull)) {
						console::log_error("Invalid hull in polygon collider!");
						continue;
					}
					b2Polygon polygon = b2MakePolygon(&hull, 0.f);
					b2CreatePolygonShape(body, &def, &polygon);
				}

			} break;
			case ObjectType::Polyline: {

				// This case is not supported.

			} break;
		}
	}

	void setup_physics(MapId map) {

		const Vec2u map_tile_size = get_tile_size(map);

		// Setup colliders for static level scenery. These are the tiles that have a TileCoord.
		for (auto [entity, tile, coord] : _registry.view<TileId, TileCoord>().each()) {

			const std::span<const ObjectId> colliders = get_objects(tile);
			if (colliders.empty())
				continue;

			// Top left corner in the tile grid.
			const Vec2f pos = {
				(float)coord.x * map_tile_size.x,
				(float)coord.y * map_tile_size.y
			};

			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_staticBody;
			body_def.position = pos;
			body_def.fixedRotation = true;
			b2BodyId body = emplace_body(entity, body_def);

			for (const ObjectId collider : colliders) {
				b2ShapeDef shape_def = b2DefaultShapeDef();
				if (get_bool(collider, "sensor")) {
					shape_def.isSensor = true;
				}
				_create_shapes(body, shape_def, collider);
			}
		}

		for (auto [entity, object] : _registry.view<ObjectId>().each()) {

			const Tag tag = get_tag(entity);
			const ObjectType type = get_type(object);

			if (type == ObjectType::Tile) {

				const TileId tile = get_tile(object);
				if (!tile) continue; // invalid tile

				const std::span<const ObjectId> objects = get_objects(tile);
				if (objects.empty()) continue; // no colliders

#if 0
				// DETERMINE PIVOT

				Vec2f pivot;

				for (ObjectId tile_object : objects) {
					if (get_type(tile_object) != ObjectType::Point)
						continue;
					if (get_name(tile_object) != "pivot")
						continue;
					pivot = get_position(tile_object);
				}
#endif

				// EMPLACE SPRITE-BODY ATTACHMENT

				make_sprite_follow_body(entity);

				// EMPLACE BODY

				b2BodyDef body_def = b2DefaultBodyDef();
				body_def.type = b2_dynamicBody;
				body_def.fixedRotation = true;
				body_def.position = get_top_left(object);
				b2BodyId body = emplace_body(entity, body_def);

				for (const ObjectId collider : objects) {

					const Vec2f collider_pos = get_position(collider); // relative to parent object
					const Vec2f collider_half_size = get_size(collider) * 0.5f;
					const Vec2f collider_center = collider_pos + collider_half_size;

					switch (get_type(collider)) {
						case ObjectType::Rectangle: {

							b2ShapeDef shape_def = b2DefaultShapeDef();
							shape_def.filter = get_physics_filter_for_tag(tag);
							b2Polygon box = b2MakeOffsetBox(
								collider_half_size.x,
								collider_half_size.y, collider_center, 0.f);
							b2CreatePolygonShape(body, &shape_def, &box);

						} break;
						case ObjectType::Ellipse: {

							b2ShapeDef shape_def = b2DefaultShapeDef();
							shape_def.filter = get_physics_filter_for_tag(tag);
							b2Circle circle{};
							circle.center = collider_center;
							circle.radius = collider_half_size.x;
							b2CreateCircleShape(body, &shape_def, &circle);

						} break;
					}
				}

				continue; // move on to next object
			}

			// Rectangle, Ellipse, Point, Polygon, Polyline

			// CREATE SENSORS

			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_staticBody;
			body_def.fixedRotation = true;
			body_def.position = get_top_left(object);
			b2BodyId body = emplace_body(entity, body_def);

			const Vec2f half_size = get_size(object) * 0.5f;
			const Vec2f center = half_size;

			switch (type) {
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
		}
	}
}