#pragma once
#include "ecs_tiled_ids.h"

namespace ecs {
	void startup_tiled();
	void shutdown_tiled();
	void setup_tiled(MapId map);

	uint8_t get_object_layer();

	/// MAPS

	MapId get_map(std::string_view path);
	std::vector<MapId> get_all_maps();
	std::string_view get_path(MapId map);
	Vector2f get_bottom_right(MapId map);

	/// TILESETS

	TilesetId get_tileset(std::string_view name);
	std::string_view get_image_path(TilesetId tileset);
	Vector2u get_size_in_tiles(TilesetId tileset);
	Vector2u get_size_in_pixels(TilesetId tileset);
	TileId get_tile(TilesetId tileset, unsigned int tile_id);

	/// TILES

	struct TextureRect {
		unsigned int x = 0; // in pixels
		unsigned int y = 0; // in pixels
		unsigned int w = 0; // in pixels
		unsigned int h = 0; // in pixels
	};

	TilesetId get_tileset(TileId tile);
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
	std::span<const Vector2f> get_points(ObjectId obj);
	std::string_view get_string(ObjectId obj, std::string_view name);
	int get_int(ObjectId obj, std::string_view name);
	float get_float(ObjectId obj, std::string_view name);
	bool get_bool(ObjectId obj, std::string_view name);
	Color get_color(ObjectId obj, std::string_view name);
	std::string_view get_file(ObjectId obj, std::string_view name);
	entt::entity get_entity(ObjectId obj, std::string_view name);
}