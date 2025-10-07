#include "stdafx.h"
#include "ecs_pathfinding.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"

namespace ecs {
	enum PathTileState : uint8_t {
		Unvisited,
		Open,
		Closed,
	};

	struct PathTile {
		PathTileState state = Unvisited;
		bool passable = true; // could be replaced by PathTileFlags in the future
		float g = FLT_MAX; // true score (as computed so far)
		float h = FLT_MAX; // heuristic score
		PathTileId parent{};
	};

	float _get_f(const PathTile* tile) {
		return tile->g + tile->h;
	}

	class PathTilePriorityQueue {
		struct CompareByF { // f = g + h
			bool operator()(PathTile* a, PathTile* b) const {
				return _get_f(a) > _get_f(b);
			}
		};

		std::vector<PathTile*> _queue;

	public:
		void push(PathTile* tile) {
			_queue.push_back(tile);
			std::push_heap(_queue.begin(), _queue.end(), CompareByF{});
		}

		void pop() {
			std::pop_heap(_queue.begin(), _queue.end(), CompareByF{});
			_queue.pop_back();
		}

		PathTile* top() {
			return _queue.front();
		}

		bool empty() const {
			return _queue.empty();
		}

		void clear() {
			_queue.clear();
		}
	};

	Vec2i _pathfinding_tile_size;
	Vec2i _pathfinding_map_size;
	std::vector<PathTile> _pathfinding_tiles; // size = _pathfinding_map_size.x * _pathfinding_map_size.y
	PathTilePriorityQueue _pathfinding_open_tiles;

	void setup_pathfinding(MapId map) {
		_pathfinding_tile_size = get_tile_size(map);
		_pathfinding_map_size = get_size_in_tiles(map);
		_pathfinding_tiles.resize(_pathfinding_map_size.x * _pathfinding_map_size.y);
		_pathfinding_open_tiles.clear(); // defensive
	}

	bool _valid_path_tile_coord(const Vec2i& coord) {
		if (coord.x < 0) return false;
		if (coord.y < 0) return false;
		if (coord.x >= _pathfinding_map_size.x) return false;
		if (coord.y >= _pathfinding_map_size.y) return false;
		return true;
	}

	PathTileId _get_path_tile_id(const Vec2i& coord) {
		return { coord.x + coord.y * _pathfinding_map_size.x };
	}

	PathTile* _get_tile(PathTileId tile) {
		return &_pathfinding_tiles[tile.id];
	}

	extern entt::registry _registry;

	void update_pathfinding(float dt) {
		// Mark tiles with colliders as non-passable.
		for (auto [entity, coord] : _registry.view<Type<Tag::Collider>, TileCoord>().each()) {
			PathTile* tile = _get_tile(_get_path_tile_id({ coord.x, coord.y }));
			tile->passable = false;
		}
	}

	bool valid(PathTileId tile) {
		return 0 <= tile.id && tile.id < _pathfinding_tiles.size();
	}

	Vec2i _get_path_tile_coord(const Vec2f& position) {
		if (_pathfinding_tile_size.x <= 0 || _pathfinding_tile_size.y <= 0) {
			return { -1, -1 };
		}
		return { (int)floor(position.x / _pathfinding_tile_size.x), // may be invalid!
			     (int)floor(position.y / _pathfinding_tile_size.y) };
	}

	PathTileId get_path_tile(const Vec2f& position) {
		Vec2i coord = _get_path_tile_coord(position);
		if (!_valid_path_tile_coord(coord))
			return {};
		return _get_path_tile_id(coord);
	}

	Vec2i _get_coord(PathTileId tile) {
		return { tile.id % _pathfinding_map_size.x,
				 tile.id / _pathfinding_map_size.x }; // SIC: x both times
	}

	Vec2i get_coord(PathTileId tile) {
		if (!valid(tile))
			return { -1, -1 };
		return _get_coord(tile);
	}

	Vec2f get_center(PathTileId tile) {
		if (!valid(tile))
			return { -1.f, -1.f };
		Vec2i coord = _get_coord(tile);
		return (Vec2f(coord) + Vec2f{ 0.5f, 0.5f }) * Vec2f(_pathfinding_tile_size);
	}

	void _reset(PathTile& tile) {
		tile.state = PathTileState::Unvisited;
		tile.g = FLT_MAX;
		tile.h = FLT_MAX;
		tile.parent = {};
	}

	int _manhattan_distance(const Vec2i& a, const Vec2i& b) {
		return abs(b.x - a.x) + abs(b.y - a.y);
	}

	float _euclidean_distance(const Vec2i& a, const Vec2i& b) {
		int dx = b.x - a.x;
		int dy = b.y - a.y;
		return sqrt((float)(dx * dx + dy * dy));
	}

	float _euclidean_distance_on_grid(const Vec2i& a, const Vec2i& b) {
		constexpr float SQRT2 = 1.41421356237f;
		int dx = abs(b.x - a.x);
		int dy = abs(b.y - a.y);
		return abs(dx - dy) + std::min(dx, dy) * SQRT2;
	}

	// The order of these directions has been chosen to minimize cache misses
	// when iterating over the array while pathfinding, so don't change it.
	constexpr Vec2i _ALLOWED_PATHFINDING_DIRECTIONS[] = {
		//Vec2i(-1, -1),
		Vec2i(0, -1),
		//Vec2i( 1, -1),
		Vec2i(-1,  0),
		Vec2i(1,  0),
		//Vec2i(-1,  1),
		Vec2i(0,  1),
		//Vec2i( 1,  1),
	};

	bool valid(std::span<const PathTileId> path) {
		for (size_t i = 0; i + 1 < path.size(); ++i) {
			if (!valid(path[i]))
				return false;
			// TODO
		}
		return false;
	}

	PathTileId _get_id(const PathTile* tile) {
		return { (int32_t)(tile - _pathfinding_tiles.data()) };
	}

	bool pathfind(PathTileId start, PathTileId end, std::vector<PathTileId>& path) {
		if (!valid(start))
			return false;
		if (!valid(end))
			return false;
		if (start == end)
			return false; // Does this make sense?

		PathTile* start_tile = _get_tile(start);
		if (!start_tile->passable)
			return false;
		PathTile* end_tile = _get_tile(end);
		if (!end_tile->passable)
			return false;

		for (PathTile& tile : _pathfinding_tiles)
			_reset(tile);

		const Vec2i start_coord = _get_coord(start);
		const Vec2i end_coord = _get_coord(end);

		start_tile->state = PathTileState::Open;
		start_tile->g = 0.f; // true score
		start_tile->h = _euclidean_distance_on_grid(start_coord, end_coord); // heuristic score

		_pathfinding_open_tiles.clear();
		_pathfinding_open_tiles.push(start_tile);

		bool path_found = false;
		while (!_pathfinding_open_tiles.empty()) {

			PathTile* tile = _pathfinding_open_tiles.top();
			if (tile == end_tile) {
				path_found = true;
				break;
			}

			_pathfinding_open_tiles.pop();
			tile->state = PathTileState::Closed;

			const PathTileId id = _get_id(tile);
			const Vec2i coord = _get_coord(id);

			for (const Vec2i& dir : _ALLOWED_PATHFINDING_DIRECTIONS) {

				const Vec2i neighbour_coord = coord + dir;
				if (!_valid_path_tile_coord(neighbour_coord))
					continue;

				PathTile* neighbor_tile = _get_tile(_get_path_tile_id(neighbour_coord));
				if (neighbor_tile->state == PathTileState::Closed)
					continue;
				if (!neighbor_tile->passable)
					continue;

				const float new_neighbor_g = tile->g + _manhattan_distance(coord, neighbour_coord);
				if (new_neighbor_g >= neighbor_tile->g)
					continue; // there's already another shorter path to this neighbor

				neighbor_tile->g = new_neighbor_g; // update true score
				neighbor_tile->parent = id;
				if (neighbor_tile->state == PathTileState::Open)
					continue;

				neighbor_tile->h = _euclidean_distance_on_grid(neighbour_coord, end_coord); // heuristic score
				neighbor_tile->state = PathTileState::Open;
				_pathfinding_open_tiles.push(neighbor_tile);
			}
		}

		_pathfinding_open_tiles.clear();

		if (!path_found)
			return false;

		path.clear();
		for (PathTileId tile = end; tile != PathTileId(); tile = _get_tile(tile)->parent)
			path.push_back(tile);
		std::reverse(path.begin(), path.end());

		return true;
	}
}