#include "stdafx.h"
#include "map_grid.h"
#include "tiled.h"
#include "console.h"

namespace map {

	extern tiled::Context _tiled_context;

	struct Tile {
		enum State : unsigned char {
			UNVISITED,
			OPEN,
			CLOSED,
		};

		Vector2i position{ -1, -1 };
		bool passable = true;
		TerrainType terrains[tiled::WangTile::COUNT] = {};
		Vector2i parent{ -1, -1 };
		float g = FLT_MAX;
		float h = FLT_MAX;
		State state = UNVISITED;
	};

	void _reset_a_star_state(Tile& tile) {
		tile.parent = Vector2i(-1, -1);
		tile.g = FLT_MAX;
		tile.h = FLT_MAX;
		tile.state = Tile::UNVISITED;
	}

	class TilePriorityQueue {
		struct CompareByF {
			bool operator()(const Tile* a, const Tile* b) const {
				return a->g + a->h > b->g + b->h;
			}
		};

		std::vector<Tile*> _queue;

	public:
		void push(Tile* tile) {
			_queue.push_back(tile);
			std::push_heap(_queue.begin(), _queue.end(), CompareByF());
		}

		void pop() {
			std::pop_heap(_queue.begin(), _queue.end(), CompareByF());
			_queue.pop_back();
		}

		Tile* top() {
			return _queue.front();
		}

		bool empty() const {
			return _queue.empty();
		}

		void clear() {
			_queue.clear();
		}
	};

	struct TileGrid {
		Vector2i size; // in tiles
		Vector2i tile_size; // in pixels
		std::vector<Tile> tiles; // tiles.size() == size.x * size.y
		TilePriorityQueue open_tiles;
	};

	// The order of these directions has been chosen to minimize cache misses
	// when iterating over the array while pathfinding, so don't change it.
	const Vector2i _ALLOWED_MOVEMENT_DIRECTIONS[] =
	{
		//Vector2i(-1, -1),
		Vector2i(0, -1),
		//Vector2i( 1, -1),
		Vector2i(-1,  0),
		Vector2i(1,  0),
		//Vector2i(-1,  1),
		Vector2i(0,  1),
		//Vector2i( 1,  1),
	};

	TileGrid _grid;

	Tile* _get_tile(const Vector2i& position) {
		if (position.x < 0) return nullptr;
		if (position.y < 0) return nullptr;
		if (position.x >= _grid.size.x) return nullptr;
		if (position.y >= _grid.size.y) return nullptr;
		return &_grid.tiles[position.x + position.y * _grid.size.x];
	}

	TerrainType _terrain_name_to_type(std::string name) // Intentional copy of name by value
	{
		name.erase(remove_if(name.begin(), name.end(), isspace), name.end());
		auto type = magic_enum::enum_cast<TerrainType>(name, magic_enum::case_insensitive);
		if (type.has_value()) return type.value();
		//console::log_error("Unknown terrain type: " + name);
		return TerrainType::None;
	}

	std::string to_string(TerrainType type) {
		return std::string(magic_enum::enum_name(type));
	}

	void create_grid(const tiled::Map& map) {
		_grid.size = Vector2i(map.width, map.height);
		_grid.tile_size = Vector2i(map.tile_width, map.tile_height);
		_grid.tiles.resize(_grid.size.x * _grid.size.y);
		_grid.open_tiles.clear();

		for (int y = 0; y < _grid.size.y; ++y) {
			for (int x = 0; x < _grid.size.x; ++x) {
				int index = x + y * _grid.size.x;
				_grid.tiles[index].position = Vector2i(x, y);
				_grid.tiles[index].passable = true;
			}
		}

		for (const tiled::Layer& layer : map.layers) {
			if (layer.tiles.size() != _grid.tiles.size())
				continue;
			if (layer.name == "Collision") {
				for (int y = 0; y < _grid.size.y; ++y) {
					for (int x = 0; x < _grid.size.x; ++x) {
						int index = x + y * _grid.size.x;
						_grid.tiles[index].passable = (layer.tiles[index].ids == 0);
					}
				}
			} else if (layer.name == "Under Sprite 1") {
				for (int y = 0; y < _grid.size.y; ++y) {
					for (int x = 0; x < _grid.size.x; ++x) {
						int index = x + y * _grid.size.x;
						if (!layer.tiles[index].ids) continue;
#if 0
						const tiled::Tile* layer_tile = tiled::find_tile_with_gid(map.tilesets, _tiled_context.tilesets, layer.tiles[index].gid);
						if (!layer_tile) continue;
						if (layer_tile->wangtiles.empty()) continue;
						const tiled::WangTile& wangtile = layer_tile->wangtiles[0];
						Tile& grid_tile = _grid.tiles[tileset];
						for (int i = 0; i < tiled::WangTile::COUNT; ++i) {
							if (!wangtile.wangcolors[i]) continue;
							grid_tile.terrains[i] = _terrain_name_to_type(wangtile.wangcolors[i]->name);
						}
#endif
					}
				}
			}
		}
	}

	void destroy_grid() {
		_grid = TileGrid();
	}

	Vector2i get_grid_size() {
		return _grid.size;
	}

	Vector2i get_tile_size() {
		return _grid.tile_size;
	}

	Vector2i world_to_tile(const Vector2f& world_pos) {
		if (!_grid.tile_size.x || !_grid.tile_size.y)
			return Vector2i(-1, -1); // Invalid tile size (grid not initialized?)
		return Vector2i(
			(int)floor(world_pos.x / _grid.tile_size.x),
			(int)floor(world_pos.y / _grid.tile_size.y));
	}

	Vector2f get_tile_center(const Vector2i& tile) {
		return Vector2f(
			(tile.x + 0.5f) * _grid.tile_size.x,
			(tile.y + 0.5f) * _grid.tile_size.y);
	}

	TerrainType get_terrain_type_at(const Vector2f& world_pos) {
		Vector2i tile_pos = world_to_tile(world_pos);
		Tile* tile = _get_tile(tile_pos);
		if (!tile) return TerrainType::None;
		const bool left = (int)world_pos.x % _grid.tile_size.x < _grid.tile_size.x / 2;
		const bool top = (int)world_pos.y % _grid.tile_size.y < _grid.tile_size.y / 2;
		const int corner =
			top ? (left ? tiled::WangTile::TOP_LEFT : tiled::WangTile::TOP_RIGHT)
			: (left ? tiled::WangTile::BOTTOM_LEFT : tiled::WangTile::BOTTOM_RIGHT);
		return tile->terrains[corner];
	}

	int _manhattan_distance(const Vector2i& a, const Vector2i& b) {
		return std::abs(b.x - a.x) + std::abs(b.y - a.y);
	}

	float _euclidean_distance(const Vector2i& a, const Vector2i& b) {
		int dx = b.x - a.x;
		int dy = b.y - a.y;
		return std::sqrt((float)(dx * dx + dy * dy));
	}

	float _euclidean_distance_on_grid(const Vector2i& a, const Vector2i& b) {
		constexpr float SQRT_2 = 1.41421356237f;
		int dx = std::abs(b.x - a.x);
		int dy = std::abs(b.y - a.y);
		return std::abs(dx - dy) + std::min(dx, dy) * SQRT_2;
	}

	bool pathfind(const Vector2i& start, const Vector2i& end, std::vector<Vector2i>& path) {
		if (start == end)
			return false; // Does this make sense?

		//TODO: better check here. we should validate that all tiles in the path are passable
		//and neighbors of each other
		if (path.size() >= 2 && path.front() == start && path.back() == end)
			return true;

		Tile* start_tile = _get_tile(start);
		if (!start_tile || !start_tile->passable)
			return false;
		Tile* end_tile = _get_tile(end);
		if (!end_tile || !end_tile->passable)
			return false;

		for (Tile& tile : _grid.tiles)
			_reset_a_star_state(tile);

		start_tile->g = 0.f;
		start_tile->h = _euclidean_distance_on_grid(start, end);
		start_tile->state = Tile::OPEN;

		_grid.open_tiles.clear();
		_grid.open_tiles.push(start_tile);

		bool path_found = false;
		while (!_grid.open_tiles.empty()) {

			Tile* current_tile = _grid.open_tiles.top();
			if (current_tile == end_tile) {
				path_found = true;
				break;
			}

			_grid.open_tiles.pop();
			current_tile->state = Tile::CLOSED;

			const Vector2i current_pos = current_tile->position;
			const float current_g = current_tile->g;

			for (const Vector2i& direction : _ALLOWED_MOVEMENT_DIRECTIONS) {

				Vector2i neighbor_pos = current_pos + direction;
				Tile* neighbor_tile = _get_tile(neighbor_pos);
				if (!neighbor_tile) continue;
				if (!neighbor_tile->passable) continue;
				if (neighbor_tile->state == Tile::CLOSED) continue;

				float tentative_neighbor_g = current_g +
					_manhattan_distance(current_pos, neighbor_pos);
				if (tentative_neighbor_g >= neighbor_tile->g) continue;

				neighbor_tile->parent = current_pos;
				neighbor_tile->g = tentative_neighbor_g;
				neighbor_tile->h = _euclidean_distance_on_grid(neighbor_pos, end);
				if (neighbor_tile->state == Tile::OPEN) continue;

				neighbor_tile->state = Tile::OPEN;
				_grid.open_tiles.push(neighbor_tile);
			}
		}

		if (!path_found)
			return false;

		path.clear();
		for (Tile* tile = end_tile; tile; tile = _get_tile(tile->parent))
			path.push_back(tile->position);
		std::reverse(path.begin(), path.end());

		return true;
	}
}