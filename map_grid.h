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

	TerrainType get_terrain_type_at(const Vec2f& world_pos);
}
