#include "stdafx.h"
#include "map_grid.h"
#include "tiled.h"

namespace map {
	struct Tile {
		TerrainType terrains[tiled::WangTile::COUNT] = {};
	};

	TerrainType _terrain_name_to_type(std::string name) {
		name.erase(remove_if(name.begin(), name.end(), isspace), name.end());
		auto type = magic_enum::enum_cast<TerrainType>(name, magic_enum::case_insensitive);
		if (type.has_value()) return type.value();
		//console::log_error("Unknown terrain type: " + name);
		return TerrainType::None;
	}

	std::string_view to_string(TerrainType type) {
		return magic_enum::enum_name(type);
	}

#if 0
	void create_grid(ecs::MapId map_id) {
		if (!map_id) return;

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
	}
#endif

	TerrainType get_terrain_type_at(const Vec2f& world_pos) {
#if 0
		Vec2i tile_pos = world_to_tile(world_pos);
		Tile* tile = _get_tile(tile_pos);
		if (!tile) return TerrainType::None;
		const bool left = (int)world_pos.x % _grid.tile_size.x < _grid.tile_size.x / 2;
		const bool top = (int)world_pos.y % _grid.tile_size.y < _grid.tile_size.y / 2;
		const int corner =
			top ? (left ? tiled::WangTile::TOP_LEFT : tiled::WangTile::TOP_RIGHT)
			: (left ? tiled::WangTile::BOTTOM_LEFT : tiled::WangTile::BOTTOM_RIGHT);
		return tile->terrains[corner];
#endif
		return TerrainType::None;
	}
}