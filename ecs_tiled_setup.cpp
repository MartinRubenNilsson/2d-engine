#include "stdafx.h"
#include "ecs_tiled.h"
#include "graphics.h"
#include "tiled.h"
#include "console.h"
#include "ecs_sprites.h"

namespace map {
	unsigned int get_object_layer_index();
}

namespace ecs {
	extern tiled::Context _tiled_context;
	extern entt::registry _registry;

	void setup_tiled(MapId map_id) {
		if (!map_id)
			return;

		const tiled::Map& map = _tiled_context.maps[map_id.id];

		// Create object entities first. This is because we want to be sure that the
		// object UIDs we get from Tiled are free to use as entity identifiers.

		for (const tiled::Layer& layer : map.layers) {
			if (layer.type != tiled::LayerType::Object)
				continue;
			for (uint32_t object_id : layer.objects) {
				const tiled::Object& object = _tiled_context.objects[object_id];

				entt::entity entity = _registry.create((entt::entity)object.id_in_map);
				assert(entity == (entt::entity)object.id_in_map);

				_registry.emplace<ObjectId>(entity, object_id);

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
					} break;
				}
			}
		}

		// Create and setup tile entities.

		for (size_t layer_index = 0; layer_index < map.layers.size(); ++layer_index) {
			const tiled::Layer& layer = map.layers[layer_index];
			if (layer.type != tiled::LayerType::Tile)
				continue;

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

					const entt::entity entity = _registry.create();

					const TilesetId tileset_id{ (uint16_t)gid.tileset_id };
					const TileId tile_id{ (uint16_t)gid.id, (uint16_t)gid.tileset_id };
					_registry.emplace<TileId>(entity, tile_id);
					_registry.emplace<Vector2u>(entity, x, y);

					const Vector2f position = {
						(float)x * map.tile_width,
						(float)y * map.tile_height - tileset.tile_height + map.tile_height
					};
					const Vector2f size = { (float)tileset.tile_width, (float)tileset.tile_height };
					const Vector2f sorting_point = { size.x / 2.f, size.y - map.tile_height / 2.f };

					// EMPLACE SPRITE

					const TextureRect tex_rect = get_texture_rect(tile_id);

					sprites::Sprite& sprite = emplace_sprite(entity);
					sprite.texture = graphics::load_texture(get_image_path(tileset_id));
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
				}
			}
		}
	}
}