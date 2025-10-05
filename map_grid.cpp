#include "stdafx.h"
#include "map_grid.h"
#include "tiled.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"

namespace ecs {
	extern entt::registry _registry;
	extern tiled::Context _tiled_context; // TODO: remove
}

namespace map {
	struct Tile {
		enum State : unsigned char {
			UNVISITED,
			OPEN,
			CLOSED,
		};

		TerrainType terrains[tiled::WangTile::COUNT] = {};
		Vec2i parent{ -1, -1 };
		float g = FLT_MAX;
		float h = FLT_MAX;
		bool passable = true;
		State state = UNVISITED;
	};

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
		Vec2i size; // in tiles
		Vec2i tile_size; // in pixels
		std::vector<Tile> tiles; // tiles.size() == size.x * size.y
		TilePriorityQueue open_tiles;
	};

	TileGrid _grid;

	Tile* _get_tile(const Vec2i& position) {
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

	void create_grid(ecs::MapId map_id) {
		if (!map_id) return;

		const tiled::Map& map = ecs::_tiled_context.maps[map_id.id];
		_grid.size = Vec2i(map.width, map.height);
		_grid.tile_size = Vec2i(map.tile_width, map.tile_height);
		_grid.tiles.resize(_grid.size.x * _grid.size.y);
		_grid.open_tiles.clear();

		for (Tile& tile : _grid.tiles) {
			tile.passable = true;
		}

		using namespace ecs;

		// Mark tiles above colliders as non-passable.
		for (auto [entity, coord] : _registry.view<Type<Tag::Collider>, TileCoord>().each()) {
			_grid.tiles[coord.x + coord.y * _grid.size.x].passable = false;
		}

#if 0
		for (const tiled::Layer& layer : map.layers) {
			if (layer.tiles.size() != _grid.tiles.size())
				continue;
			else if (layer.name == "Under Sprite 1") {
				for (int y = 0; y < _grid.size.y; ++y) {
					for (int x = 0; x < _grid.size.x; ++x) {
						int index = x + y * _grid.size.x;
						if (!layer.tiles[index].ids) continue;
						const tiled::Tile* layer_tile = tiled::find_tile_with_gid(map.tilesets, _tiled_context.tilesets, layer.tiles[index].gid);
						if (!layer_tile) continue;
						if (layer_tile->wangtiles.empty()) continue;
						const tiled::WangTile& wangtile = layer_tile->wangtiles[0];
						Tile& grid_tile = _grid.tiles[tileset];
						for (int i = 0; i < tiled::WangTile::COUNT; ++i) {
							if (!wangtile.wangcolors[i]) continue;
							grid_tile.terrains[i] = _terrain_name_to_type(wangtile.wangcolors[i]->name);
						}
					}
				}
			}
		}
#endif
	}

	void destroy_grid() {
		_grid = TileGrid();
	}

	Vec2i get_grid_size() {
		return _grid.size;
	}

	Vec2i get_tile_size() {
		return _grid.tile_size;
	}

	Vec2i world_to_tile(const Vec2f& world_pos) {
		if (!_grid.tile_size.x || !_grid.tile_size.y)
			return Vec2i(-1, -1); // Invalid tile size (grid not initialized?)
		return Vec2i(
			(int)floor(world_pos.x / _grid.tile_size.x),
			(int)floor(world_pos.y / _grid.tile_size.y));
	}

	Vec2f get_tile_center(const Vec2i& tile) {
		return Vec2f(
			(tile.x + 0.5f) * _grid.tile_size.x,
			(tile.y + 0.5f) * _grid.tile_size.y);
	}

	TerrainType get_terrain_type_at(const Vec2f& world_pos) {
		Vec2i tile_pos = world_to_tile(world_pos);
		Tile* tile = _get_tile(tile_pos);
		if (!tile) return TerrainType::None;
		const bool left = (int)world_pos.x % _grid.tile_size.x < _grid.tile_size.x / 2;
		const bool top = (int)world_pos.y % _grid.tile_size.y < _grid.tile_size.y / 2;
		const int corner =
			top ? (left ? tiled::WangTile::TOP_LEFT : tiled::WangTile::TOP_RIGHT)
			: (left ? tiled::WangTile::BOTTOM_LEFT : tiled::WangTile::BOTTOM_RIGHT);
		return tile->terrains[corner];
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
		constexpr float SQRT_2 = 1.41421356237f;
		int dx = abs(b.x - a.x);
		int dy = abs(b.y - a.y);
		return abs(dx - dy) + std::min(dx, dy) * SQRT_2;
	}

	void _reset_a_star_state(Tile& tile) {
		tile.parent = { -1, -1 };
		tile.g = FLT_MAX;
		tile.h = FLT_MAX;
		tile.state = Tile::UNVISITED;
	}

	Vec2i _get_position(const Tile* tile) {
		int i = tile - _grid.tiles.data();
		return { i % _grid.size.x, i / _grid.size.x };
	}

	// The order of these directions has been chosen to minimize cache misses
	// when iterating over the array while pathfinding, so don't change it.
	constexpr Vec2i _ALLOWED_MOVEMENT_DIRECTIONS[] = {
		//Vec2i(-1, -1),
		Vec2i(0, -1),
		//Vec2i( 1, -1),
		Vec2i(-1,  0),
		Vec2i(1,  0),
		//Vec2i(-1,  1),
		Vec2i(0,  1),
		//Vec2i( 1,  1),
	};

	bool pathfind(const Vec2i& start, const Vec2i& end, std::vector<Vec2i>& path) {
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

			Tile* tile = _grid.open_tiles.top();
			if (tile == end_tile) {
				path_found = true;
				break;
			}

			_grid.open_tiles.pop();
			tile->state = Tile::CLOSED;

			const Vec2i pos = _get_position(tile);

			for (const Vec2i& dir : _ALLOWED_MOVEMENT_DIRECTIONS) {

				const Vec2i neighbor_pos = pos + dir;
				Tile* neighbor_tile = _get_tile(neighbor_pos);
				if (!neighbor_tile)
					continue;
				if (neighbor_tile->state == Tile::CLOSED)
					continue;
				if (!neighbor_tile->passable)
					continue;

				const float new_neighbor_g = tile->g + _manhattan_distance(pos, neighbor_pos);
				if (new_neighbor_g >= neighbor_tile->g)
					continue;

				neighbor_tile->parent = pos;
				neighbor_tile->g = new_neighbor_g;
				neighbor_tile->h = _euclidean_distance_on_grid(neighbor_pos, end);
				if (neighbor_tile->state == Tile::OPEN)
					continue;

				neighbor_tile->state = Tile::OPEN;
				_grid.open_tiles.push(neighbor_tile);
			}
		}

		if (!path_found)
			return false;

		path.clear();
		for (Tile* tile = end_tile; tile; tile = _get_tile(tile->parent))
			path.push_back(_get_position(tile));
		std::reverse(path.begin(), path.end());

		return true;
	}
}