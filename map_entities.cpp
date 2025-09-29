#include "stdafx.h"
#include "map.h"
#include "map_entities.h"

#include "tiled.h"
#include "tiled_types.h"

#include "console.h"
#include "graphics.h"
#include "graphics_globals.h"

#include "ecs.h"
#include "ecs_tiled.h"
#include "ecs_common.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_damage.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"
#include "ecs_player.h"
#include "ecs_camera.h"
#include "ecs_ai.h"
#include "ecs_portal.h"
#include "ecs_chest.h"

#include "player_outfit.h"

namespace map {
	unsigned int get_object_layer_index();

	extern tiled::Context _tiled_context;

	void destroy_entities() {
		ecs::clear();
	}

	void create_entities(const tiled::Map& map) {
		//
		// IMPORTANT:
		// 
		// In one of my school projects, my group had a similar function to this one,
		// with the difference being that we used functions like entt::registry::emplace()
		// directly. It turns out that this caused a lot of template bloat, which slowed down
		// compilation times, and eventually made the .cpp file too large to compile.
		// 
		// The solution was to wrap calls to entt::registry::emplace() in our own helper functions
		// and put them in separate .cpp files, so each template is instantiated in its own unit.
		// This is why we have functions like ecs::emplace_tile() and ecs::emplace_body().
		//
		// TLDR: You may NOT access entt::registry directly in this file! Make a helper function
		// in ecs_[your component here].cpp and call that instead!
		//

		const Vector2f map_bounds_min = { 0.f, 0.f };
		const Vector2f map_bounds_max = {
			(float)map.width * map.tile_width,
			(float)map.height * map.tile_height };

		//TODO: clean this up

		// Save some components to be carried over between maps,
		// or that is needed in general when populating the new map.
		std::optional<ecs::Player> last_player;
		std::optional<ecs::Portal> last_active_portal;
		{
			entt::entity player_entity = ecs::find_player_entity();
			if (ecs::Player* player = ecs::get_player(player_entity)) {
				last_player = *player;
			}
			entt::entity portal_entity = ecs::find_active_portal_entity();
			if (ecs::Portal* portal = ecs::get_portal(portal_entity)) {
				last_active_portal = *portal;
			}
		}

		// Destroy all entities before creating new ones.

		destroy_entities();

		// Pre-create object entities. This is because we want to be sure that the
		// object UIDs we get from Tiled are free to use as entity identifiers.

		for (const tiled::Layer& layer : map.layers) {
			if (layer.type != tiled::LayerType::Object) continue;
			for (const tiled::Object& object : layer.objects) {
				ecs::create((entt::entity)object.id);
			}
		}

		// Setup object entities.

		for (const tiled::Layer& layer : map.layers) {
			if (layer.type != tiled::LayerType::Object) continue;
			for (const tiled::Object& object : layer.objects) {

				// At this point, a corresponding entity should have been created
				// that reuses the object UID from Tiled. If not, something went wrong.
				entt::entity entity = (entt::entity)object.id;
				assert(ecs::valid(entity));

				if (!object.name.empty()) {
					ecs::set_name(entity, object.name);
				}

				ecs::Tag tag = ecs::Tag::None;
				if (!object.class_.empty() && ecs::string_to_tag(object.class_, tag)) {
					ecs::set_tag(entity, tag);
				}

				ecs::emplace_tiled_object(entity, object);

				if (!object.properties.empty()) {
					ecs::set_properties(entity, object.properties);
				}

				// In Tiled, objects are positioned by their top-left corner...
				Vector2f position_top_left = Vector2f(object.x, object.y);

				switch (object.type) {
				case tiled::ObjectType::Tile: {

					// ...unless it's a tile, in which case it's positioned by its bottom-left corner.
					// This is confusing, so let's adjust the position here to make it consistent.
					position_top_left.y -= object.height;

					const tiled::Tileset* tileset = get_tileset(object.tileset.tileset_id);
					if (!tileset) {
						console::log_error("Tileset not found for object " + object.name);
						continue;
					}

					const unsigned int tile_id = object.tile.gid - object.tileset.first_gid;
					if (tile_id >= tileset->tiles.size()) {
						console::log_error("Tile not found for object " + object.name);
						continue;
					}

					const tiled::Tile& tile = tileset->tiles[tile_id];

					// EMPLACE SPRITE

					tiled::TextureRect tex_rect = tiled::get_tile_texture_rect(*tileset, tile_id);

					sprites::Sprite& sprite = ecs::emplace_sprite(entity);
					sprite.texture = graphics::load_texture(tileset->image_path);
					sprite.position = position_top_left;
					sprite.size.x = object.width;
					sprite.size.y = object.height;
					sprite.tex_position = { (float)tex_rect.x, (float)tex_rect.y };
					sprite.tex_size = { (float)tex_rect.w, (float)tex_rect.h };
					Vector2u texture_size;
					graphics::get_texture_size(sprite.texture, texture_size.x, texture_size.y);
					sprite.tex_position /= Vector2f(texture_size);
					sprite.tex_size /= Vector2f(texture_size);
					// PITFALL: We don't set the sorting layer to the layer index here.
					// This is because we want all objects to be on the same layer, so they
					// are rendered in the correct order. This sorting layer may also be the
					// index of a tile layer so that certain static tiles are rendered as if
					// they were objects, e.g. trees and other props.
					sprite.sorting_layer = (uint8_t)get_object_layer_index();
					sprite.sorting_point = Vector2f(object.width / 2.f, object.height / 2.f);
					if (!layer.visible) {
						sprite.flags &= ~sprites::SPRITE_VISIBLE;
					}
					if (object.tile.flipped_horizontally) {
						sprite.flags |= sprites::SPRITE_FLIP_HORIZONTALLY;
					}
					if (object.tile.flipped_vertically) {
						sprite.flags |= sprites::SPRITE_FLIP_VERTICALLY;
					}
					if (object.tile.flipped_diagonally) {
						sprite.flags |= sprites::SPRITE_FLIP_DIAGONALLY;
					}

					// EMPLACE ANIMATION

					ecs::TileAnimation& animation = ecs::emplace_tile_animation(entity);
					animation.tileset_id = object.tileset.tileset_id;
					animation.tile_id = tile_id;

					if (!tile.objects.empty()) {

						// DETERMINE PIVOT

						Vector2f pivot;

						for (const tiled::Object& object : tile.objects) {
							if (object.type != tiled::ObjectType::Point) continue;
							if (object.name != "pivot") continue;
							pivot.x = object.x;
							pivot.y = object.y;
						}

						sprite.sorting_point = pivot;

						// EMPLACE SPRITE-BODY ATTACHMENT

						ecs::make_sprite_follow_body(entity);

						// EMPLACE BODY

						b2BodyDef body_def = b2DefaultBodyDef();
						body_def.type = b2_dynamicBody;
						body_def.fixedRotation = true;
						body_def.position = position_top_left;
						b2BodyId body = ecs::emplace_body(entity, body_def);

						for (const tiled::Object& collider : tile.objects) {

							const float coll_x = collider.x;
							const float coll_y = collider.y;
							const float coll_hw = collider.width / 2.f;
							const float coll_hh = collider.height / 2.f;
							const Vector2f coll_center = { coll_x + coll_hw, coll_y + coll_hh };

							switch (collider.type) {
							case tiled::ObjectType::Rectangle: {

								b2ShapeDef shape_def = b2DefaultShapeDef();
								shape_def.filter = ecs::get_physics_filter_for_tag(tag);
								b2Polygon box = b2MakeOffsetBox(coll_hw, coll_hh, coll_center, 0.f);
								b2CreatePolygonShape(body, &shape_def, &box);

							} break;
							case tiled::ObjectType::Ellipse: {

								b2ShapeDef shape_def = b2DefaultShapeDef();
								shape_def.filter = ecs::get_physics_filter_for_tag(tag);
								b2Circle circle{};
								circle.center = coll_center;
								circle.radius = coll_hw;
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
					body_def.position = position_top_left;
					b2BodyId body = ecs::emplace_body(entity, body_def);

					const float hw = object.width / 2.f;
					const float hh = object.height / 2.f;
					const Vector2f center = { hw, hh };

					switch (object.type) {
					case tiled::ObjectType::Rectangle: {

						b2ShapeDef shape_def = b2DefaultShapeDef();
						shape_def.isSensor = true;
						shape_def.filter = ecs::get_physics_filter_for_tag(tag);
						b2Polygon box = b2MakeOffsetBox(hw, hh, center, 0.f);
						b2CreatePolygonShape(body, &shape_def, &box);

					} break;
					case tiled::ObjectType::Ellipse: {

						b2ShapeDef shape_def = b2DefaultShapeDef();
						shape_def.isSensor = true;
						shape_def.filter = ecs::get_physics_filter_for_tag(tag);
						b2Circle circle{};
						circle.center = center;
						circle.radius = hw;
						b2CreateCircleShape(body, &shape_def, &circle);

					} break;
					}

				} break;
				}

				// TAG-SPECIFIC ENTITY SETUP

				switch (tag) {
				case ecs::Tag::Player: {

					ecs::Player player{};
					if (last_player) {
						player = *last_player;
						player.input_flags = 0;
					}

					const Vector2f pivot = { 32.f, 42.f };

					{
						player::Outfit outfit{};
						player::randomize_outfit(outfit);
						player::create_outfit_texture(outfit);
					}

					if (sprites::Sprite* sprite = ecs::get_sprite(entity)) {
						sprite->texture = graphics::get_framebuffer_texture(graphics::player_outfit_framebuffer);
						sprite->sorting_point = pivot;
					}
					{
						b2BodyDef body_def = b2DefaultBodyDef();
						body_def.type = b2_dynamicBody;
						body_def.position = position_top_left + pivot;
						body_def.fixedRotation = true;
						b2BodyId body = ecs::emplace_body(entity, body_def);
						b2ShapeDef shape_def = b2DefaultShapeDef();
						shape_def.filter = ecs::get_physics_filter_for_tag(tag);
						b2Circle circle{};
						circle.radius = 7.f;
						b2CreateCircleShape(body, &shape_def, &circle);
					}

					ecs::make_sprite_follow_body(entity, -pivot);

					if (last_active_portal) {

						if (const tiled::Object* target_point = tiled::find_object_with_name(map, last_active_portal->target_point)) {
#if 0
							if (body) {
								body->SetTransform(target_point->position_top_left, 0.f);
							}
#endif
						}

						if (last_active_portal->exit_direction == "up") {
							player.look_dir = { 0.f, -1.f };
						} else if (last_active_portal->exit_direction == "down") {
							player.look_dir = { 0.f, 1.f };
						} else if (last_active_portal->exit_direction == "left") {
							player.look_dir = { -1.f, 0.f };
						} else if (last_active_portal->exit_direction == "right") {
							player.look_dir = { 1.f, 0.f };
						}
					}

					player.held_item = ecs::create();
					ecs::emplace_tile_animation(player.held_item);
					ecs::emplace_player(entity, player);

					ecs::set_physics_event_handler(entity, ecs::on_player_physics_event);

					ecs::set_damage_handler(entity, ecs::apply_damage_to_player);

					ecs::Camera camera{};
					camera.center = position_top_left;
					camera.confines_min = map_bounds_min;
					camera.confines_max = map_bounds_max;
					camera.entity_to_follow = entity;
					ecs::emplace_camera(entity, camera);
					ecs::activate_camera(entity, true);

				} break;
				case ecs::Tag::Slime: {

					ecs::emplace_ai(entity, ecs::AiType::Slime);
					ecs::set_damage_handler(entity, ecs::apply_damage_to_slime);

				} break;
				case ecs::Tag::Portal: {

					ecs::Portal& portal = ecs::emplace_portal(entity);
					get<tiled::PropertyType::String>(object.properties, "target_map", portal.target_map);
					get<tiled::PropertyType::String>(object.properties, "target_point", portal.target_point);
					get<tiled::PropertyType::String>(object.properties, "exit_direction", portal.exit_direction);

				} break;
				}
			}
		}

		// Create and setup tile entities.

		for (size_t layer_index = 0; layer_index < map.layers.size(); ++layer_index) {
			const tiled::Layer& layer = map.layers[layer_index];
			if (layer.type != tiled::LayerType::Tile) continue;

			// OPTIMIZATION: When iterating through the view of all ecs::Tile components, EnTT
			// returns them in reverse order of creation. Let's therefore CREATE them in reverse
			// draw order (bottom-to-top and right-to-left) so that when we iterate we access them
			// in draw order (left-to-right and top-to-bottom). This makes it so we spend less time
			// sorting them before rendering.

			for (unsigned int y = layer.height; y--;) {
				for (unsigned int x = layer.width; x--;) {

					const tiled::TileGid tile_gid = layer.tiles[x + y * layer.width];
					if (!tile_gid.gid) continue; // Skip empty tiles

					const tiled::TilesetLink tileset_link = tiled::find_tileset_link_for_tile_gid(map.tilesets, tile_gid.gid);
					if (!tileset_link.first_gid) continue; // Valid GIDs start at 1

					const tiled::Tileset* tileset = get_tileset(tileset_link.tileset_id);
					if (!tileset) {
						console::log_error("Tileset not found for GID " + std::to_string(tile_gid.gid));
						continue;
					}

					const unsigned int tile_id = tile_gid.gid - tileset_link.first_gid;
					if (tile_id >= tileset->tiles.size()) {
						console::log_error("Tile not found for GID " + std::to_string(tile_gid.gid));
						continue;
					}

					const tiled::Tile& tile = tileset->tiles[tile_id];

					const Vector2f position = {
						(float)x * map.tile_width,
						(float)y * map.tile_height - tileset->tile_height + map.tile_height
					};
					const Vector2f size = { (float)tileset->tile_width, (float)tileset->tile_height };
					const Vector2f sorting_point = { size.x / 2.f, size.y - map.tile_height / 2.f };

					entt::entity entity = ecs::create();
					ecs::Tag tag = ecs::Tag::None;
					if (!tile.class_.empty() && ecs::string_to_tag(tile.class_, tag)) {
						ecs::set_tag(entity, tag);
					}
					if (!tile.properties.empty()) {
						ecs::set_properties(entity, tile.properties);
					}

					// EMPLACE SPRITE

					tiled::TextureRect tex_rect = tiled::get_tile_texture_rect(*tileset, tile_id);

					sprites::Sprite& sprite = ecs::emplace_sprite(entity);
					sprite.texture = graphics::load_texture(tileset->image_path);
					sprite.position = position;
					sprite.size = size;
					sprite.tex_position = { (float)tex_rect.x, (float)tex_rect.y };
					sprite.tex_size = { (float)tex_rect.w, (float)tex_rect.h };
					Vector2u texture_size;
					graphics::get_texture_size(sprite.texture, texture_size.x, texture_size.y);
					sprite.tex_position /= Vector2f(texture_size);
					sprite.tex_size /= Vector2f(texture_size);
					sprite.sorting_layer = (uint8_t)layer_index;
					sprite.sorting_point = sorting_point;
					if (!layer.visible) {
						sprite.flags &= ~sprites::SPRITE_VISIBLE;
					}
					if (tile_gid.flipped_horizontally) {
						sprite.flags |= sprites::SPRITE_FLIP_HORIZONTALLY;
					}
					if (tile_gid.flipped_vertically) {
						sprite.flags |= sprites::SPRITE_FLIP_VERTICALLY;
					}
					if (tile_gid.flipped_diagonally) {
						sprite.flags |= sprites::SPRITE_FLIP_DIAGONALLY;
					}

					// EMPLACE ANIMATION

					// The majority of tiles are not animated and don't change during gameplay,
					// so let's only add an animation component if the tile is actually animated.
					if (!tile.animation.empty()) {
						ecs::TileAnimation& animation = ecs::emplace_tile_animation(entity);
						animation.tileset_id = tileset_link.tileset_id;
						animation.tile_id = tile_id;
					}

					// EMPLACE BODY

					// PITFALL: We only create bodies for tiles that have colliders!
					b2BodyId body = b2_nullBodyId;

					if (!tile.objects.empty()) {

						b2BodyDef body_def = b2DefaultBodyDef();
						body_def.type = b2_staticBody;
						body_def.position = position;
						body_def.fixedRotation = true;
						body = ecs::emplace_body(entity, body_def);

						for (const tiled::Object& collider : tile.objects) {

							const Vector2f collider_center(collider.x, collider.y);
							const Vector2f collider_half_size(collider.width / 2.f, collider.height / 2.f);

							b2ShapeDef shape_def = b2DefaultShapeDef();
							get<tiled::PropertyType::Bool>(collider.properties, "sensor", shape_def.isSensor);

							switch (collider.type) {
							case tiled::ObjectType::Rectangle: {

								b2Polygon box = b2MakeOffsetBox(
									collider_half_size.x,
									collider_half_size.y,
									collider_center + collider_half_size, 0.f);
								b2CreatePolygonShape(body, &shape_def, &box);

							} break;
							case tiled::ObjectType::Ellipse: {

								b2Circle circle{};
								circle.center = collider_center;
								circle.radius = collider_half_size.x;
								b2CreateCircleShape(body, &shape_def, &circle);

							} break;
							case tiled::ObjectType::Polygon: {

								const std::span<const Vector2f> points{ (const Vector2f*)collider.points.data(), collider.points.size() };
								const int32_t count = (int32_t)points.size();
								if (count < 3) {
									console::log_error("Too few points in polygon collider! Got " + std::to_string(count) + ", need >= 3.");
									break;
								}

								if (count <= b2_maxPolygonVertices && is_convex(points)) {

									b2Vec2 polygon_points[b2_maxPolygonVertices];
									for (int32_t i = 0; i < count; ++i) {
										polygon_points[i] = collider_center + points[i];
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
										triangle_points[j] = collider_center + triangles[i + j];
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
							case tiled::ObjectType::Point: {

								sprite.sorting_point = Vector2f(collider.x, collider.y);

							} break;
							}
						}
					}
				}
			}
		}

		ecs::setup();

		// PITFALL: ecs::setup() may want to query data from Tiled objects,
		// so we must call it before ecs::clear_all_tiled_objects()! 
		ecs::clear_all_tiled_objects();
	}

	void patch_entities(const MapPatch& patch) {
		// Patching is a way to modify the state of the map after it has been created.

		for (entt::entity entity : patch.destroyed_entities) {
			ecs::destroy_immediately(entity);
		}
		for (entt::entity entity : patch.opened_chests) {
			ecs::open_chest(entity, true);
		}
	}
}