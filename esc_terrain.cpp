#include "stdafx.h"
#include "ecs_terrain.h"
#include "ecs_tiled.h"

namespace ecs {
	std::string_view to_string(TerrainType type) {
		return magic_enum::enum_name(type);
	}

	TerrainType to_terrain_type(std::string_view string) {
		std::string copy{ string };
		copy.erase(std::remove_if(copy.begin(), copy.end(), std::not_fn(isalpha)), copy.end());
		auto type = magic_enum::enum_cast<TerrainType>(copy, magic_enum::case_insensitive);
		if (type.has_value())
			return type.value();
		return TerrainType::None;
	}
	
	Vec2u _terrain_tile_size; // in pixels; half the map tile size
	Vec2u _terrain_size; // in terrain tiles; twice the map size in tiles
	std::vector<TerrainType> _terrain; // size = _terrain_size.x * _terrain_size.y

	unsigned int _terrain_tile_coord_to_id(const Vec2u& coord) {
		return coord.x + coord.y * _terrain_size.x;
	}

	extern entt::registry _registry;

	void setup_terrain(MapId map) {
		// We subdivide each map tile into 4 terrain tiles, one for each corner,
		// hence the terrain size in tiles becomes twice the map size.

		_terrain_tile_size = get_tile_size(map) / 2u;
		_terrain_size = get_size_in_tiles(map) * 2u;

		for (auto [entity, tile, coord] : _registry.view<TileId, TileCoord>().each()) {
			// This will be the terrain coord of the top left corner.
			const Vec2u terrain_tile_coord = { 2u * coord.x, 2u * coord.y };
			const unsigned int terrain_tile_id = _terrain_tile_coord_to_id(terrain_tile_coord);

		}

#if 0
		for (const tiled::Layer& layer : map.layers) {
			if (layer.tiles.size() != _grid.tiles.size())
				continue;
			else if (layer.name == "Under Sprite 1") {
				for (int y = 0; y < _grid.size.y; ++y) {
					for (int x = 0; x < _grid.size.x; ++x) {
						int index = x + y * _grid.size.x;
						if (!layer.tiles[index].ids) continue;
						const tiled::Tile* layer_tile = tiled::find_tile_with_gid(map.tilesets, _tiled_context.tilesets, layer.tiles[index].gid);
						if (!layer_tile) continue;
						if (layer_tile->wangtiles.empty()) continue;
						const tiled::WangTile& wangtile = layer_tile->wangtiles[0];
						Tile& grid_tile = _grid.tiles[tileset];
						for (int i = 0; i < tiled::WangTile::COUNT; ++i) {
							if (!wangtile.wangcolors[i]) continue;
							grid_tile.terrains[i] = _terrain_name_to_type(wangtile.wangcolors[i]->name);
						}
					}
				}
			}
		}
#endif
	}
}