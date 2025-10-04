#pragma once

namespace ecs {
	void startup_tiled_maps();
	void shutdown_tiled_maps();

	struct MapId {
		uint16_t id = UINT16_MAX;

		operator bool() const; // checks if the ID is valid
		auto operator<=>(const MapId&) const = default;
	};

	struct TilesetId {
		uint16_t id = UINT16_MAX;

		operator bool() const; // checks if the ID is valid
		auto operator<=>(const TilesetId&) const = default;
	};

	struct TileId {
		uint16_t id = UINT16_MAX;
		uint16_t tileset_id = UINT16_MAX;

		operator bool() const; // checks if both IDs are valid
		auto operator<=>(const TileId&) const = default;
	};

	struct ObjectId {
		uint32_t id = UINT32_MAX;

		operator bool() const; // checks if the ID is valid
		auto operator<=>(const ObjectId&) const = default;
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

	std::string_view get_image_path(TilesetId tileset);
	Handle<graphics::Texture> get_texture(TilesetId tileset);
	TileId get_tile(TilesetId tileset, unsigned int tile_id);

	/// TILES

	struct TextureRect {
		unsigned int x = 0; // in pixels
		unsigned int y = 0; // in pixels
		unsigned int w = 0; // in pixels
		unsigned int h = 0; // in pixels
	};

	std::string_view get_class(TileId tile);
	std::span<const ObjectId> get_objects(TileId tile);
	TextureRect get_texture_rect(TileId tile);
	bool animated(TileId tile);
	unsigned int get_animation_duration(TileId tile); // duration in milliseconds
	TileId get_animation_frame(TileId tile, unsigned int time_ms); // time in milliseconds
	TileId change(TileId tile, unsigned int id);
	TileId offset(TileId tile, int delta_x, int delta_y);

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