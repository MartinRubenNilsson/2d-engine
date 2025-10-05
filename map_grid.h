#pragma once
#include "ecs_tiled_ids.h"

namespace map {
	enum class TerrainType {
		None,
		Dirt,
		LightGrass,
		DarkGrass,
		Cobblestone,
		ShallowWater,
		DeepWater,
	};

	std::string to_string(TerrainType type);

	void create_grid(ecs::MapId map);
	void destroy_grid();

	Vec2i get_grid_size(); // in tiles
	Vec2i get_tile_size(); // in pixels
	Vec2i world_to_tile(const Vec2f& world_pos);
	Vec2f get_tile_center(const Vec2i& tile);
	TerrainType get_terrain_type_at(const Vec2f& world_pos);
	bool pathfind(const Vec2i& start, const Vec2i& end, std::vector<Vec2i>& path);
}
