#pragma once

namespace tiled {
	struct Map;
	struct Tileset;
	struct Tile;
	struct Object;
}

namespace ecs {
	class TMap;
	class TTile;
	class TObject;

	// Wrapper class for safe and convenient access to a Tiled map.
	class TMap {
		const tiled::Map* _map;

	public:
		TMap(const tiled::Map* map);

		// NOTE: The top left is always (0, 0).
		Vector2f get_bottom_right() const;
	};

	// Wrapper class for safe and convenient access to a Tiled tile.
	class TTile {
		const tiled::Tile* _tile;
		const tiled::Tileset* _tileset;

	public:
		TTile(const tiled::Tile* tile, const tiled::Tileset* tileset);
	};

	enum class TObjectType { // must match tiled::ObjectType!
		Rectangle,
		Ellipse,
		Point,
		Polygon,
		Polyline,
		Tile,
		Text, // not supported right now
	};

	// Wrapper class for safe and convenient access to a Tiled object.
	class TObject {
		const tiled::Object* _obj;
		const tiled::Map* _map;

	public:
		TObject(const tiled::Object* obj, const tiled::Map* map);

		TMap get_map() const;

		entt::entity get_id() const;
		TObjectType get_type() const;

		// Returns the position of the top-left corner, unless the object is a tile object,
		// in which case it returns the position of the bottom-left corner.
		Vector2f get_position() const;
		Vector2f get_top_left() const;

		std::span<const Vector2f> get_points() const;

		std::string_view get_string(std::string_view name) const;
		int get_int(std::string_view name) const;
		float get_float(std::string_view name) const;
		bool get_bool(std::string_view name) const;
		Color get_color(std::string_view name) const;
		std::string_view get_file(std::string_view name) const;
		entt::entity get_object(std::string_view name) const;
	};

	void emplace_tiled_tile(entt::entity entity, const TTile& tile);
	void emplace_tiled_object(entt::entity entity, const TObject& obj);
	void clear_all_tiled_tiles_and_objects();
}