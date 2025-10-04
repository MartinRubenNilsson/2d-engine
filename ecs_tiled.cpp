#include "stdafx.h"
#include "ecs_tiled.h"
#include "tiled.h"
#include "console.h"
#include "graphics.h"
#include "map.h" // TODO: remove
#include "filesystem.h"

// TODO: move into own files
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"

namespace ecs {
	bool _tiled_file_load_callback(std::string_view path, std::string& contents) {
		return filesystem::read_text_file(path, contents);
	}

	void _tiled_debug_message_callback(std::string_view message) {
		__debugbreak();
		console::log_error(message);
	}

	tiled::Context _tiled_context;
	MapId _current_tiled_map{};

	void startup_tiled_maps() {
		_tiled_context.file_load_callback = _tiled_file_load_callback;
		_tiled_context.debug_message_callback = _tiled_debug_message_callback;

		// Preload all Tiled assets.

		const std::span<const filesystem::File> files = filesystem::get_all_files_in_directory("assets/tiled");
		// Load tilesets first...
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledTileset) continue;
			tiled::load_tileset_from_file(_tiled_context, file.path);
		}
		// ...then templates...
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledTemplate) continue;
			tiled::load_object_from_file(_tiled_context, file.path);
		}
		// ...and lastly maps.
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledMap) continue;
			tiled::load_map_from_file(_tiled_context, file.path);
		}

		_current_tiled_map = {};
	}

	void shutdown_tiled_maps() {
		_current_tiled_map = {};
		_tiled_context = {};
	}

	MapId get_current_map() {
		return _current_tiled_map;
	}

	MapId get_map(std::string_view path) {
		for (uint16_t i = 0; i < _tiled_context.maps.size(); ++i) {
			std::string_view tentative_path = _tiled_context.maps[i].path;
			if (tentative_path == path) {
				return { .id = i };
			} else if (filesystem::get_stem(tentative_path) == filesystem::get_stem(path)) {
				return { .id = i };
			}
		}
		return {};
	}

	std::vector<MapId> get_all_maps() {
		std::vector<MapId> maps(_tiled_context.maps.size());
		for (uint16_t i = 0; i < maps.size(); ++i) {
			maps[i] = { .id = i };
		}
		return maps;
	}

	const tiled::Map* _get_map(MapId id) {
		if (id.id >= _tiled_context.maps.size()) return nullptr;
		return &_tiled_context.maps[id.id];
	}

	bool valid(MapId map) {
		return map.id < _tiled_context.maps.size();
	}

	std::string_view get_path(MapId map) {
		if (const tiled::Map* m = _get_map(map)) {
			return m->path;
		}
		return {};
	}

	Vector2f get_bottom_right(MapId map) {
		if (const tiled::Map* m = _get_map(map)) {
			return { (float)m->width * m->tile_width,
					 (float)m->height * m->tile_height };
		}
		return {};
	}

	TilesetId get_tileset(std::string_view name) {
		if (name.empty()) return {};
		for (uint16_t i = 0; i < _tiled_context.tilesets.size(); ++i) {
			if (_tiled_context.tilesets[i].name == name) {
				return { .id = i };
			}
		}
		return {};
	}

	bool valid(TilesetId tileset) {
		return tileset.id < _tiled_context.tilesets.size();
	}

	std::string_view get_image_path(TilesetId tileset) {
		return _tiled_context.tilesets[tileset.id].image_path;
	}

	bool valid(TileId tile) {
		return tile.tileset_id < _tiled_context.tilesets.size() &&
			tile.id < _tiled_context.tilesets[tile.tileset_id].tiles.size();
	}

	const tiled::Tile& _get_tile(TileId tile) {
		return _tiled_context.tilesets[tile.tileset_id].tiles[tile.id];
	}

	TextureRect get_texture_rect(TileId tile) {
		const tiled::Tileset& ts = _tiled_context.tilesets[tile.tileset_id];
		TextureRect rect{};
		rect.x = (tile.id % ts.columns) * (ts.tile_width + ts.spacing) + ts.margin;
		rect.y = (tile.id / ts.columns) * (ts.tile_height + ts.spacing) + ts.margin;
		rect.w = ts.tile_width;
		rect.h = ts.tile_height;
		return rect;
	}

	template <tiled::PropertyType type>
	struct GetPropertyRetVal {
		using Type = std::variant_alternative_t<(size_t)type, tiled::PropertyValue>;
	};

	template <>
	struct GetPropertyRetVal<tiled::PropertyType::String> {
		using Type = std::string_view;
	};

	template <>
	struct GetPropertyRetVal<tiled::PropertyType::File> {
		using Type = std::string_view;
	};

	template <>
	struct GetPropertyRetVal<tiled::PropertyType::Class> {
		using Type = std::string_view;
	};

	const tiled::Object& _get_object(ObjectId obj) {
		return _tiled_context.objects[obj.id];
	}

	template <tiled::PropertyType type>
	GetPropertyRetVal<type>::Type _get_property(ObjectId obj, std::string_view name) {
		const tiled::Object& object = _get_object(obj);
		constexpr size_t index = (size_t)type;
		for (const tiled::Property& prop : object.properties) {
			if (prop.value.index() != index) continue;
			if (prop.name != name) continue;
			return std::get<index>(prop.value);
		}
		return {};
	}

	bool valid(ObjectId obj) {
		return obj.id < _tiled_context.objects.size();
	}

	entt::entity get_entity(ObjectId obj) {
		return (entt::entity)_get_object(obj).id;
	}

	ObjectType get_type(ObjectId obj) {
		return (ObjectType)_get_object(obj).type;
	}

	Vector2f get_position(ObjectId obj) {
		const tiled::Object& object = _get_object(obj);
		return { object.x, object.y };
	}

	Vector2f get_top_left(ObjectId obj) {
		const tiled::Object& object = _get_object(obj);
		Vector2f p = { object.x, object.y };
		if (object.type == tiled::ObjectType::Tile) {
			p.y -= object.height;
		}
		return p;
	}

	std::string_view get_string(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::String>(obj, name);
	}

	int get_int(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::Int>(obj, name);
	}

	float get_float(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::Float>(obj, name);
	}

	bool get_bool(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::Bool>(obj, name);
	}

	Color get_color(ObjectId obj, std::string_view name) {
		tiled::Color color = _get_property<tiled::PropertyType::Color>(obj, name);
		return { color.r, color.g, color.b, color.a };
	}

	std::string_view get_file(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::File>(obj, name);
	}

	entt::entity get_entity(ObjectId obj, std::string_view name) {
		return (entt::entity)_get_property<tiled::PropertyType::Object>(obj, name);
	}

	extern entt::registry _registry;

	bool setup_tiled_map(std::string_view path) {

		_current_tiled_map = get_map(path);
		const tiled::Map* map = _get_map(_current_tiled_map);
		if (!map) return false;

		// Create object entities first. This is because we want to be sure that the
		// object UIDs we get from Tiled are free to use as entity identifiers.

		for (const tiled::Layer& layer : map->layers) {
			if (layer.type != tiled::LayerType::Object) continue;
			for (const tiled::Object& object : layer.objects) {

				entt::entity entity = _registry.create((entt::entity)object.id);
				assert(entity == (entt::entity)object.id);

				//const ObjectId obj_id{ .id = (uint16_t)std::distance(layer.objects.data(), &object) };
				//_registry.emplace<ObjectId>(entity, obj_id);

				// TODO: Move to setup_tags()
				Tag tag = Tag::None;
				if (!object.class_.empty() && string_to_tag(object.class_, tag)) {
					set_tag(entity, tag);
				}

				// In Tiled, objects are positioned by their top-left corner...
				Vector2f position_top_left = Vector2f(object.x, object.y);

				switch (object.type) {
					case tiled::ObjectType::Tile: {

						// ...unless it's a tile, in which case it's positioned by its bottom-left corner.
						// This is confusing, so let's adjust the position here to make it consistent.
						position_top_left.y -= object.height;

						if (object.tile.tileset_id >= _tiled_context.tilesets.size()) {
							console::log_error("Tileset not found for object " + object.name);
							continue;
						}

						const tiled::Tileset& tileset = _tiled_context.tilesets[object.tile.tileset_id];

						if (object.tile.id >= tileset.tiles.size()) {
							console::log_error("Tile not found for object " + object.name);
							continue;
						}

						const tiled::Tile& tile = tileset.tiles[object.tile.id];
						const TileId tile_id{ (uint16_t)object.tile.id, (uint16_t)object.tile.tileset_id };

						// EMPLACE SPRITE

						const TextureRect tex_rect = get_texture_rect(tile_id);

						sprites::Sprite& sprite = emplace_sprite(entity);
						sprite.texture = graphics::load_texture(tileset.image_path);
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
						sprite.sorting_layer = (uint8_t)map::get_object_layer_index();
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
						{
							TileAnimation& animation = emplace_tile_animation(entity);
							animation.tileset_id = object.tile.tileset_id;
							animation.tile_id = object.tile.id;
						}

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

							make_sprite_follow_body(entity);

							// EMPLACE BODY

							b2BodyDef body_def = b2DefaultBodyDef();
							body_def.type = b2_dynamicBody;
							body_def.fixedRotation = true;
							body_def.position = position_top_left;
							b2BodyId body = emplace_body(entity, body_def);

							for (const tiled::Object& collider : tile.objects) {

								const float coll_x = collider.x;
								const float coll_y = collider.y;
								const float coll_hw = collider.width / 2.f;
								const float coll_hh = collider.height / 2.f;
								const Vector2f coll_center = { coll_x + coll_hw, coll_y + coll_hh };

								switch (collider.type) {
									case tiled::ObjectType::Rectangle: {

										b2ShapeDef shape_def = b2DefaultShapeDef();
										shape_def.filter = get_physics_filter_for_tag(tag);
										b2Polygon box = b2MakeOffsetBox(coll_hw, coll_hh, coll_center, 0.f);
										b2CreatePolygonShape(body, &shape_def, &box);

									} break;
									case tiled::ObjectType::Ellipse: {

										b2ShapeDef shape_def = b2DefaultShapeDef();
										shape_def.filter = get_physics_filter_for_tag(tag);
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
						b2BodyId body = emplace_body(entity, body_def);

						const float hw = object.width / 2.f;
						const float hh = object.height / 2.f;
						const Vector2f center = { hw, hh };

						switch (object.type) {
							case tiled::ObjectType::Rectangle: {

								b2ShapeDef shape_def = b2DefaultShapeDef();
								shape_def.isSensor = true;
								shape_def.filter = get_physics_filter_for_tag(tag);
								b2Polygon box = b2MakeOffsetBox(hw, hh, center, 0.f);
								b2CreatePolygonShape(body, &shape_def, &box);

							} break;
							case tiled::ObjectType::Ellipse: {

								b2ShapeDef shape_def = b2DefaultShapeDef();
								shape_def.isSensor = true;
								shape_def.filter = get_physics_filter_for_tag(tag);
								b2Circle circle{};
								circle.center = center;
								circle.radius = hw;
								b2CreateCircleShape(body, &shape_def, &circle);

							} break;
						}

					} break;
				}
			}
		}

		// Create and setup tile entities.

		for (size_t layer_index = 0; layer_index < map->layers.size(); ++layer_index) {
			const tiled::Layer& layer = map->layers[layer_index];
			if (layer.type != tiled::LayerType::Tile) continue;

			// OPTIMIZATION: When iterating through the view of all Tile components, EnTT
			// returns them in reverse order of creation. Let's therefore CREATE them in reverse
			// draw order (bottom-to-top and right-to-left) so that when we iterate we access them
			// in draw order (left-to-right and top-to-bottom). This makes it so we spend less time
			// sorting them before rendering.

			for (unsigned int y = layer.height; y--;) {
				for (unsigned int x = layer.width; x--;) {

					const tiled::TileGid gid = layer.tiles[x + y * layer.width];
					if (gid.tileset_id >= _tiled_context.tilesets.size()) {
						continue;
					}
					const tiled::Tileset& tileset = _tiled_context.tilesets[gid.tileset_id];
					if (gid.id >= tileset.tiles.size()) {
						continue;
					}
					const tiled::Tile& tile = tileset.tiles[gid.id];

					entt::entity entity = _registry.create();

					const TileId tile_id{ (uint16_t)gid.id, (uint16_t)gid.tileset_id };
					_registry.emplace<TileId>(entity, tile_id);

					const Vector2f position = {
						(float)x * map->tile_width,
						(float)y * map->tile_height - tileset.tile_height + map->tile_height
					};
					const Vector2f size = { (float)tileset.tile_width, (float)tileset.tile_height };
					const Vector2f sorting_point = { size.x / 2.f, size.y - map->tile_height / 2.f };

					// TODO: move to setup_tags()
					Tag tag = Tag::None;
					if (!tile.class_.empty() && string_to_tag(tile.class_, tag)) {
						set_tag(entity, tag);
					}

					// EMPLACE SPRITE

					const TextureRect tex_rect = get_texture_rect(tile_id);

					sprites::Sprite& sprite = emplace_sprite(entity);
					sprite.texture = graphics::load_texture(tileset.image_path);
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
					if (gid.flipped_horizontally) {
						sprite.flags |= sprites::SPRITE_FLIP_HORIZONTALLY;
					}
					if (gid.flipped_vertically) {
						sprite.flags |= sprites::SPRITE_FLIP_VERTICALLY;
					}
					if (gid.flipped_diagonally) {
						sprite.flags |= sprites::SPRITE_FLIP_DIAGONALLY;
					}

					// EMPLACE ANIMATION

					// The majority of tiles are not animated and don't change during gameplay,
					// so let's only add an animation component if the tile is actually animated.
					if (!tile.animation.empty()) {
						TileAnimation& animation = emplace_tile_animation(entity);
						animation.tileset_id = gid.tileset_id;
						animation.tile_id = gid.id;
					}

					// EMPLACE BODY

					// PITFALL: We only create bodies for tiles that have colliders!
					b2BodyId body = b2_nullBodyId;

					if (!tile.objects.empty()) {

						b2BodyDef body_def = b2DefaultBodyDef();
						body_def.type = b2_staticBody;
						body_def.position = position;
						body_def.fixedRotation = true;
						body = emplace_body(entity, body_def);

						for (const tiled::Object& collider : tile.objects) {

							const Vector2f collider_center(collider.x, collider.y);
							const Vector2f collider_half_size(collider.width / 2.f, collider.height / 2.f);

							b2ShapeDef shape_def = b2DefaultShapeDef();
							//get<tiled::PropertyType::Bool>(collider.properties, "sensor", shape_def.isSensor);

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

		return true;
	}
}
