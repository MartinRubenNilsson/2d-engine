#pragma once

namespace ecs {
	enum class TerrainType : uint8_t {
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

	TerrainType get_terrain_at(const Vec2f& position);

	void setup_terrain(MapId map);
	void clear_terrain();
	void debug_draw_terrain();
}