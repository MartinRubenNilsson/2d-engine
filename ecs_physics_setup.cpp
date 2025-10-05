#include "stdafx.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_sprites.h"
#include "console.h"

namespace ecs {
	extern entt::registry _registry;

	void setup_physics(MapId map) {
		for (auto [entity, object] : _registry.view<ObjectId>().each()) {

			const Tag tag = get_tag(entity);
			const ObjectType object_type = get_type(object);

			if (object_type == ObjectType::Tile) {

				const TileId tile = get_tile(object);
				if (!tile) continue; // invalid tile

				const std::span<const ObjectId> objects = get_objects(tile);
				if (objects.empty()) continue;

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

				for (ObjectId collider : objects) {

					const Vec2f pos = get_position(collider);
					const Vec2f half_size = get_size(collider) * 0.5f;
					const Vec2f center = pos + half_size;

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

			switch (object_type) {
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

		const Vec2u map_tile_size = get_tile_size(map);

		for (auto [entity, tile, tile_pos] : _registry.view<TileId, TileCoord>().each()) {
			if (!tile)
				continue;

			const std::span<const ObjectId> colliders = get_objects(tile);
			if (colliders.empty())
				continue;

			// Top left corner in the tile grid.
			const Vec2f position = {
				(float)tile_pos.x * map_tile_size.x,
				(float)tile_pos.y * map_tile_size.y
			};

			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_staticBody;
			body_def.position = position;
			body_def.fixedRotation = true;
			b2BodyId body = emplace_body(entity, body_def);

			for (const ObjectId collider : colliders) {
				const Vec2f center = get_position(collider);
				const Vec2f half_size = get_size(collider) * 0.5f;

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

						const std::span<const Vec2f> points = get_points(collider);
						const int32_t count = (int32_t)points.size();
						if (count < 3) {
							console::log_error("Too few points in polygon collider! Got " + std::to_string(count) + ", need >= 3.");
							break;
						}

						if (count <= b2_maxPolygonVertices && convex(points)) {

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
						const std::vector<Vec2f> triangles = triangulate(points);
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
				}
			}
		}
	}
}