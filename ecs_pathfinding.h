#pragma once

namespace ecs {
	void setup_pathfinding(MapId map);
	void clear_pathfinding();
	void update_pathfinding(float dt);

	struct PathTileId {
		int32_t id = -1;
		auto operator<=>(const PathTileId&) const = default;
	};

	bool valid(PathTileId tile);
	PathTileId get_path_tile(const Vec2f& position);
	Vec2i get_coord(PathTileId tile);
	Vec2f get_center(PathTileId tile);

	// Checks that the path is traversable, but (PITFALL!) not that it is the shortest.
	bool valid(std::span<const PathTileId> path);
	bool pathfind(PathTileId start, PathTileId end, std::vector<PathTileId>& path);
}