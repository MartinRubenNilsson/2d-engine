#include "stdafx.h"
#include "ecs_tiled.h"
#include "tiled.h"
#include "console.h"

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

				const ObjectId object{ .id = object_id };

				const entt::entity desired_entity = get_entity(object);
				const entt::entity entity = _registry.create(desired_entity);
				if (entity != desired_entity) {
					console::log_error("Error 10757: Failed to preserve object UID when creating entity");
					_registry.destroy(entity);
					continue;
				}

				_registry.emplace<ObjectId>(entity, object);

				if (get_type(object) != ObjectType::Tile)
					continue;
				const TileId tile = get_tile(object);
				if (!tile) {
					console::log_error("Error 10391: Invalid tile for tile object");
					continue;
				}

				_registry.emplace<TileId>(entity, tile);
			}
		}

		// Create and setup tile entities.

		for (uint16_t layer_id = 0; layer_id < map.layers.size(); ++layer_id) {
			const tiled::Layer& layer = map.layers[layer_id];
			if (layer.type != tiled::LayerType::Tile)
				continue;

			// OPTIMIZATION: When iterating through the view of all Tile components, EnTT
			// returns them in reverse order of creation. Let's therefore CREATE them in reverse
			// draw order (bottom-to-top and right-to-left) so that when we iterate we access them
			// in draw order (left-to-right and top-to-bottom). This makes it so we spend less time
			// sorting them before rendering (in theory).
			//
			// (Notice that map.layers are in top-to-bottom order, so we're already iterating them
			// in reverse draw order, which is what we want here.)

			for (uint16_t y = layer.height; y--;) {
				for (uint16_t x = layer.width; x--;) {

					// TODO: flip flags
					const tiled::TileGid gid = layer.tiles[x + y * layer.width];
					const TileId tile{
						.id = (uint16_t)gid.id,
						.tileset_id = (uint16_t)gid.tileset_id
					};

					if (!tile) continue;

					const entt::entity entity = _registry.create();

					_registry.emplace<TileId>(entity, tile);
					_registry.emplace<TileCoord>(entity, x, y, layer_id);
				}
			}
		}
	}
}