#pragma once

namespace ecs {
	void startup_tiled_maps();
	void shutdown_tiled_maps();

	struct MapId {
		uint16_t id = UINT16_MAX;
	};

	struct TilesetId {
		uint16_t id = UINT16_MAX;
	};

	struct TileId {
		uint16_t id = UINT16_MAX;
		uint16_t tileset_id = UINT16_MAX;
	};

	struct ObjectId {
		uint32_t id = UINT32_MAX;
	};

	/// MAPS

	// Gets the current map as set by the last call to setup_tiled_map().
	MapId get_current_map();
	MapId get_map(std::string_view path);
	std::vector<MapId> get_all_maps();

	bool valid(MapId map);
	std::string_view get_path(MapId map);
	Vector2f get_bottom_right(MapId map);

	/// TILESETS

	TilesetId get_tileset(std::string_view name);

	bool valid(TilesetId tileset);
	std::string_view get_image_path(TilesetId tileset);

	/// TILES

	struct TextureRect {
		unsigned int x = 0; // in pixels
		unsigned int y = 0; // in pixels
		unsigned int w = 0; // in pixels
		unsigned int h = 0; // in pixels
	};

	bool valid(TileId tile);
	std::string_view get_class(TileId tile);
	std::span<const ObjectId> get_objects(TileId tile);
	TextureRect get_texture_rect(TileId tile);
	bool animated(TileId tile);

	/// OBJECTS

	enum class ObjectType { // must match tiled::ObjectType!
		Rectangle,
		Ellipse,
		Point,
		Polygon,
		Polyline,
		Tile,
		Text, // not supported right now
	};

	bool valid(ObjectId obj);
	entt::entity get_entity(ObjectId obj);
	ObjectType get_type(ObjectId obj);
	std::string_view get_name(ObjectId obj);
	std::string_view get_class(ObjectId obj);
	TileId get_tile(ObjectId obj);
	// PITFALL: For tile objects this is the bottom left!
	Vector2f get_position(ObjectId obj);
	Vector2f get_top_left(ObjectId obj); // in world space
	Vector2f get_size(ObjectId obj);
	std::string_view get_string(ObjectId obj, std::string_view name);
	int get_int(ObjectId obj, std::string_view name);
	float get_float(ObjectId obj, std::string_view name);
	bool get_bool(ObjectId obj, std::string_view name);
	Color get_color(ObjectId obj, std::string_view name);
	std::string_view get_file(ObjectId obj, std::string_view name);
	entt::entity get_entity(ObjectId obj, std::string_view name);

	bool setup_tiled_map(std::string_view path);
}