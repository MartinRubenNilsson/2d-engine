#include "stdafx.h"
#include "ecs_tiled.h"
#include "tiled.h"
#include "console.h"
#include "ecs_sprites.h"

namespace ecs {
	extern tiled::Context _tiled_context;
	extern entt::registry _registry;
	uint8_t _object_layer = 0;

	uint8_t get_object_layer() {
		return _object_layer;
	}

	size_t _find_object_layer(std::span<const tiled::Layer> layers) {
		// 1. Look for a tile layer whose name starts with "object".
		for (size_t i = 0; i < layers.size(); ++i) {
			if (layers[i].type != tiled::LayerType::Tile)
				continue;
			if (layers[i].name.starts_with("object") ||
				layers[i].name.starts_with("Object")) {
				return i;
			}
		}
		// 2. Look for the topmost object layer.
		for (size_t i = 0; i < layers.size(); ++i) {
			if (layers[i].type == tiled::LayerType::Object)
				return i;
		}
		// 3. Return the topmost layer.
		return 0;
	}

	void setup_tiled(MapId map_id) {
		if (!map_id)
			return;

		const tiled::Map& map = _tiled_context.maps[map_id.id];

		// Determine the "object layer". This is not necessarily an actual object layer,
		// rather the purpose is to have everything that counts as an "object" be put on
		// the same layer in order to have correct rendering. This includes all "normal"
		// objects, but also tiles on certain tile layers.

		_object_layer = (uint8_t)_find_object_layer(map.layers);

		// Create object entities first. This is because we want to be sure that the
		// object UIDs we get from Tiled are free to use as entity identifiers.

		for (const tiled::Layer& layer : map.layers) {
			if (layer.type != tiled::LayerType::Object)
				continue;
			for (uint32_t object_id : layer.objects) {
				if (object_id >= _tiled_context.objects.size()) {
					console::log_error("Error 19175: Failed to find object in map");
					continue;
				}
				const tiled::Object& object = _tiled_context.objects[object_id];
				entt::entity entity = _registry.create((entt::entity)object.uid);
				if (entity != (entt::entity)object.uid) {
					console::log_error("Error 10757: Failed to preserve object UID when creating entity");
					_registry.destroy(entity);
					continue;
				}
				_registry.emplace<ObjectId>(entity, object_id);
			}
		}

		// Create and setup tile entities.

		for (uint8_t layer_id = 0; layer_id < map.layers.size(); ++layer_id) {
			const tiled::Layer& layer = map.layers[layer_id];
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
					update_sprite(sprite, tile_id);
					sprite.sorting_layer = (uint8_t)layer_id;
					sprite.sorting_point = sorting_point;
					sprite.position = position;
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