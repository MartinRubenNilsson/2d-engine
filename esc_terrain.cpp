#include "stdafx.h"
#include "ecs_terrain.h"
#include "ecs_tiled.h"
#include "text.h"
#include "fonts.h"

namespace ecs {
	std::string_view to_string(TerrainType type) {
		return magic_enum::enum_name(type);
	}

	TerrainType to_terrain_type(std::string_view string) {
		if (string.empty())
			return TerrainType::None;
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
		_terrain.resize(_terrain_size.x * _terrain_size.y);

		for (auto [entity, tile, coord] : _registry.view<TileId, TileCoord>().each()) {
			// This will be the terrain coord of the top left corner.
			const Vec2u top_left_corner_coord = { 2u * coord.x, 2u * coord.y };
			const unsigned int top_left_corner_id = _terrain_tile_coord_to_id(top_left_corner_coord);
			const unsigned int all_corner_ids[4] = {
				top_left_corner_id + 1, // top-right corner
				top_left_corner_id + 1 + _terrain_size.x, // bottom-right corner
				top_left_corner_id + _terrain_size.x, // bottom-left corner
				top_left_corner_id, // top-left corner
			};
			std::string_view terrain_names[MAX_TERRAINS_PER_TILE];
			get_terrain_names(tile, terrain_names);
			for (unsigned int i = 0; i < 4; ++i) { // iterate over all four corners
				// PITFALL: In the terrain_names array, 0 is the top edge and 1 is the top-right corner,
				// and then the remaining edges and corners keep alternating clockwise.
				std::string_view terrain_name = terrain_names[2 * i + 1];
				if (terrain_name.empty()) continue;
				const TerrainType type = to_terrain_type(terrain_name);
				if (type == TerrainType::None) continue;
				_terrain[all_corner_ids[i]] = type;
			}
		}
	}

	void debug_draw_terrain() {
		text::Text text{};
		text.font = fonts::load_font("assets/fonts/Helvetica.ttf");;
		text.pixel_height = 48.f;
		text.scale = { 0.1f, 0.1f };

		for (unsigned int y = 0; y < _terrain_size.y; ++y) {
			for (unsigned int x = 0; x < _terrain_size.x; ++x) {
				const Vec2u coord = { x, y };
				const unsigned int id = _terrain_tile_coord_to_id(coord);
				const TerrainType type = _terrain[id];
				if (type == TerrainType::None) continue;
				text.string = text::to_u32(std::to_string((int)type));
				text.position = coord * _terrain_tile_size;
				text.position.x += 2.5f;
				text.position.y += 5.5f;
				text::draw(text);
			}
		}
	}
}