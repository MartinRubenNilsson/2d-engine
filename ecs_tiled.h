#pragma once
#include "ecs_tiled_ids.h"

namespace ecs {
	void startup_tiled();
	void shutdown_tiled();
	void setup_tiled(MapId map);

	uint8_t get_object_layer();

	/// WORLDS

	WorldId get_world(std::string_view path);
	MapId get_map_at_position(WorldId world, const Vec2i& position); // position in pixels
	Vec2i get_position_of_map(WorldId world, MapId map); // position in pixels

	/// MAPS

	MapId get_map(std::string_view path);
	std::vector<MapId> get_all_maps();
	std::string_view get_path(MapId map);
	Vec2u get_tile_size(MapId map); // in pixels
	Vec2u get_size_in_tiles(MapId map);
	Vec2u get_size_in_pixels(MapId map);
	bool is_layer_visible(MapId map, unsigned int layer);

	/// TILESETS

	TilesetId get_tileset(std::string_view name);
	std::string_view get_name(TilesetId tileset);
	std::string_view get_class(TilesetId tileset);
	std::string_view get_image_path(TilesetId tileset);
	Vec2u get_tile_size(TilesetId tileset); // in pixels
	Vec2u get_size_in_tiles(TilesetId tileset);
	Vec2u get_size_in_pixels(TilesetId tileset);
	TileId get_tile(TilesetId tileset, unsigned int tile_id);

	/// TILES

	struct TileCoord {
		uint16_t x = 0; // horizontal position in tile grid
		uint16_t y = 0; // vertical position in tile grid
		uint16_t layer = 0;
	};

	constexpr size_t MAX_TERRAINS_PER_TILE = 8;

	TilesetId get_tileset(TileId tile);
	std::string_view get_class(TileId tile);
	std::span<const ObjectId> get_objects(TileId tile);
	Vec2u get_size(TileId tile); // in pixels
	Rect2u get_texture_rect(TileId tile); // returns texture rect *in pixel coordinates*
	bool animated(TileId tile);
	unsigned int get_animation_duration_ms(TileId tile); // in milliseconds
	float get_animation_duration(TileId tile); // in seconds

	struct TileAnimationFrame {
		unsigned int index = UINT_MAX;
		TileId tile{};
	};

	// preserves the flip flags of the input tile
	TileAnimationFrame get_animation_frame(TileId tile, unsigned int time_ms); // time in milliseconds
	void replace(TileId& tile, unsigned int id); // preserves flip flags
	void replace_step(TileId& tile, int step_x, int step_y); // preserves flip flags
	// Returns the names of the terrains in the different parts of tile. (Only looks at the first terrain set.)
	// The returned order of parts is:
	//   1. top edge
	//   2. top-right corner
	//   3. right edge
	//   4. bottom-right corner
	//   5. bottom edge
	//   6. bottom-left corner
	//   7. left edge
	//   8. top-left corner
	void get_terrain_names(TileId tile, std::span<std::string_view, MAX_TERRAINS_PER_TILE> terrain_names);

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
	Vec2f get_position(ObjectId obj);
	Vec2f get_top_left(ObjectId obj); // in world space
	Vec2f get_center(ObjectId obj);
	Vec2f get_size(ObjectId obj);
	std::span<const Vec2f> get_points(ObjectId obj);
	std::string_view get_string(ObjectId obj, std::string_view name);
	int get_int(ObjectId obj, std::string_view name);
	float get_float(ObjectId obj, std::string_view name);
	bool get_bool(ObjectId obj, std::string_view name);
	Color get_color(ObjectId obj, std::string_view name);
	std::string_view get_file(ObjectId obj, std::string_view name);
	entt::entity get_entity(ObjectId obj, std::string_view name);
}