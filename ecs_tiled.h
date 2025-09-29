#pragma once

namespace tiled {
	struct Map;
	struct Tile;
	struct Object;
}

namespace ecs {
	class TiledMap;
	class TiledTile;
	class TiledObject;

	// Wrapper class for safe and convenient access to a Tiled map.
	class TiledMap {
		const tiled::Map* _map;

	public:
		TiledMap(const tiled::Map* map);

		// NOTE: The top left is always (0, 0).
		Vector2f get_bottom_right() const;
	};

	// Wrapper class for safe and convenient access to a Tiled tile.
	class TiledTile {
		const tiled::Tile* _tile;

	public:
		TiledTile(const tiled::Tile* tile);
	};

	enum class ObjectType { // must match tiled::ObjectType!
		Rectangle,
		Ellipse,
		Point,
		Polygon,
		Polyline,
		Tile,
		Text, // not supported right now
	};

	// Wrapper class for safe and convenient access to a Tiled object.
	class TiledObject {
		const tiled::Object* _obj;
		const tiled::Map* _map;

	public:
		TiledObject(const tiled::Object* obj, const tiled::Map* map);

		TiledMap get_map() const;

		entt::entity get_id() const;
		ObjectType get_type() const;

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

	void emplace_tiled_object(entt::entity entity, const TiledObject& obj);
	void clear_all_tiled_objects();
}