#include "stdafx.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "console.h"

namespace ecs {
	extern entt::registry _registry;

	void _create_shapes(b2BodyId body, const b2ShapeDef& def, ObjectId object, bool object_is_in_local_space) {

		// PITFALL: Calling get_position() for non-tile objects returns the top left, but
		// for tile objects it returns the bottom left! get_top_left() was made to fix this.
		const Vec2f top_left = object_is_in_local_space ? get_top_left(object) : Vec2f::ZERO;
		const Vec2f half_size = get_size(object) * 0.5f;
		const Vec2f center = top_left + half_size;

		switch (get_type(object)) {
			case ObjectType::Rectangle: {

				b2Polygon box = b2MakeOffsetBox(half_size.x, half_size.y, center, b2Rot_identity);
				b2CreatePolygonShape(body, &def, &box);

			} break;
			case ObjectType::Ellipse: {

				b2Circle circle{};
				circle.center = center;
				circle.radius = half_size.x; // Box2D doesn't support ellipses, so this is the best we can do.
				b2CreateCircleShape(body, &def, &circle);

			} break;
			case ObjectType::Point: {

				// Not supported.

			} break;
			case ObjectType::Polygon: {

				const std::span<const Vec2f> points = get_points(object);
				const int32_t num_points = (int32_t)points.size();
				if (num_points < 3) {
					console::log_error("Too few points in polygon collider! Got " + std::to_string(num_points) + ", need >= 3.");
					break;
				}

				// Box2D supports convex polygons with a small maximum number of points.

				if (num_points <= B2_MAX_POLYGON_VERTICES && convex(points)) {

					b2Vec2 polygon_points[B2_MAX_POLYGON_VERTICES];
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

				// Either the polygon have too many vertices or it's concave.
				// In either case we need to split it into smaller convex shapes.

				//TODO: fix triangulate()! it has broken math.
				//TODO: make triangulate() return index buffer?
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

				// Not supported.

			} break;
			case ObjectType::Text: {

				// Not supported.

			} break;
		}
	}

	void setup_physics(MapId map) {

		const Vec2u map_tile_size = get_tile_size(map);

		// Setup static colliders for the level scenery. These are the tiles that have a TileCoord.
		for (auto [entity, tile, coord] : _registry.view<TileId, TileCoord>().each()) {

			const std::span<const ObjectId> colliders = get_objects(tile);
			if (colliders.empty())
				continue;

			const Vec2u size = get_size(tile);

			// PITFALL: The tile size may be greater (or smaller) than the tile grid used by the map!
			// In that case the tile's *bottom left* is aligned to the grid cell's bottom left. This
			// means that the tile may extend *above* (or below) the grid cell. We need to compensate
			// for this when we position the body.

			const float height_overshoot = (float)size.y - map_tile_size.y;
			const Vec2f top_left = {
				(float)coord.x * map_tile_size.x,
				(float)coord.y * map_tile_size.y - height_overshoot // compensate for overshoot
			};

			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_staticBody;
			body_def.position = top_left;
			body_def.fixedRotation = true;
			b2BodyId body = emplace_body(entity, body_def);

			for (const ObjectId collider : colliders) {
				b2ShapeDef shape_def = b2DefaultShapeDef();
				if (get_bool(collider, "sensor")) {
					shape_def.isSensor = true;
					shape_def.enableSensorEvents = true;
				}
				_create_shapes(body, shape_def, collider, true);
			}
		}

		// Setup colliders for objects. Tile objects are assumed to be dynamic,
		// while other objects are assumed to be static sensors.
		for (auto [entity, object] : _registry.view<ObjectId>().each()) {

			const Tag tag = get_tag(entity);
			const ObjectType type = get_type(object);

			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.position = get_top_left(object);
			body_def.fixedRotation = true;

			b2ShapeDef shape_def = b2DefaultShapeDef();
			shape_def.filter = get_physics_filter(tag);

			if (type == ObjectType::Tile) {

				const TileId tile = get_tile(object);
				if (!tile)
					continue; // invalid tile

				const std::span<const ObjectId> colliders = get_objects(tile);
				if (colliders.empty())
					continue; // no colliders
				
				body_def.type = b2_dynamicBody;
				b2BodyId body = emplace_body(entity, body_def);

				shape_def.enableContactEvents = true;

				for (const ObjectId collider : colliders) {
					_create_shapes(body, shape_def, collider, true);
				}

				continue; // move on to next object
			}

			// Rectangle, Ellipse, Point, Polygon, Polyline, Text

			body_def.type = b2_staticBody;
			b2BodyId body = emplace_body(entity, body_def);

			shape_def.isSensor = true;
			shape_def.enableSensorEvents = true;

			_create_shapes(body, shape_def, object, false);
		}
	}
}