#pragma once

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

	std::string_view to_string(TerrainType type);
	TerrainType get_terrain_type_at(const Vec2f& world_pos);
}
