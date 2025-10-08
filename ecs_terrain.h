#pragma once

namespace ecs {
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
	TerrainType to_terrain_type(std::string_view string);

	void setup_terrain(MapId map);
}