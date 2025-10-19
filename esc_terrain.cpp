#include "stdafx.h"
#include "ecs_terrain.h"
#include "ecs_tiled.h"
#include "text.h"
#include "text_fonts.h"
#include "graphics_debugging.h"

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

	Vec2u _position_to_terrain_tile_coord(const Vec2f& position) {
		if (_terrain_tile_size.x == 0 || _terrain_tile_size.y == 0)
			return Vec2u::ZERO;
		return floor(position / (Vec2f)_terrain_tile_size);
	}

	TerrainType get_terrain_at(const Vec2f& position) {
		const unsigned int id = _terrain_tile_coord_to_id(_position_to_terrain_tile_coord(position));
		if (id >= _terrain.size())
			return TerrainType::None;
		return _terrain[id];
	}

	extern entt::registry _registry;

	void setup_terrain(MapId map) {

		// We subdivide each map tile into 4 terrain tiles, one for each corner,
		// hence the terrain size in tiles becomes twice the map size.
		_terrain_tile_size = get_tile_size(map) / 2u;
		_terrain_size = get_size_in_tiles(map) * 2u;
		_terrain.resize(_terrain_size.x * _terrain_size.y);

		const uint8_t object_layer = get_object_layer();

		for (auto [entity, tile, coord] : _registry.view<TileId, TileCoord>().each()) {
			// PITFALL: We don't want tiles on layers *above* where entities can walk to override
			// those on lower layers. Hence, let's skip all layers above the object layer.
			if (coord.layer > object_layer)
				continue;
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

	void clear_terrain() {
		_terrain_tile_size = {};
		_terrain_size = {};
		_terrain.clear();
	}

	std::string_view _to_index_string(TerrainType type) {
		const int n = (int)type;
		static char buffer[2];
		if (n < 0) {
			return {};
		}
		if (n < 10) {
			buffer[0] = '0' + n;
			return { buffer, 1 };
		}
		return {};
	}

	void debug_draw_terrain(const Rect2f& view) {
		if (_terrain_size.x == 0 || _terrain_size.y == 0)
			return;

		GRAPHICS_DEBUG_GROUP;

		Vec2u min = _position_to_terrain_tile_coord(view.min);
		Vec2u max = _position_to_terrain_tile_coord(view.max);
		max = ::min(max, _terrain_size - Vec2u(1, 1));
		min = ::min(min, max);

		text::Text text{};
		text.font = text::load_font("assets/fonts/Helvetica.ttf");
		text.font_size = 8.f;
		text.anchor = text::TextAnchor::MiddleCenter;

		for (unsigned int y = min.y; y <= max.y; ++y) {
			for (unsigned int x = min.x; x <= max.x; ++x) {

				const Vec2u coord = { x, y };
				const unsigned int id = _terrain_tile_coord_to_id(coord);
				const TerrainType type = _terrain[id];
				if (type == TerrainType::None)
					continue;

				text.string = _to_index_string(type);
				text.position = coord;
				text.position += Vec2f(0.5f, 0.5f);
				text.position *= Vec2f(_terrain_tile_size);
				// TODO: color

				text::draw_later(text);
			}
		}

		text::draw_all_now();
	}
}