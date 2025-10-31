#pragma once
#include <variant>
#include <vector>
#include <string>

namespace tiled {
	struct Color {
		unsigned char r = 0;
		unsigned char g = 0;
		unsigned char b = 0;
		unsigned char a = 0;
	};

	enum class PropertyType {
		String,
		Int,
		Float,
		Bool,
		Color,
		File,
		Object,
		Class
	};

	using PropertyValue = std::variant<
		std::string, // string
		int, // int
		float, // float
		bool, // bool
		Color, // color
		std::string, // file
		unsigned int, // object
		std::string // class
	>;

	struct Property {
		std::string name;
		PropertyValue value;
	};

	// A 32-bit integer that stores a pair of IDs in the lower 28 bits and flip flags in the upper 4 bits.
	// These TileId:s are global in the sense that they may refer to a tile from any tileset.
	//
	struct TileId {
		union {
			uint32_t value = 0x0FFFFFFF; // set flip flags to 0 and remaining bits to 1
			struct {
				uint32_t id : 16; // index into Tileset::tiles[]
				uint32_t tileset_id : 12; // index into Context::tilesets[]
				uint32_t rotated_hexagonal_120 : 1; // only for hexagonal maps
				uint32_t flipped_diagonally : 1;
				uint32_t flipped_vertically : 1;
				uint32_t flipped_horizontally : 1;
			};
		};
	};

	enum class ObjectType {
		Rectangle,
		Ellipse,
		Point,
		Polygon,
		Polyline,
		Tile,
		Text, // not supported right now
	};

	struct Point {
		float x = 0.f;
		float y = 0.f;
	};

	struct Object {
		// This is a unique ID for the object among all objects in the map
		// (and NOT an index into Context::objects[]). Valid UIDs are >= 1.
		unsigned int uid = 0; 
		ObjectType type = ObjectType::Rectangle;
		std::string name;
		std::string class_;
		std::vector<Property> properties;
		std::vector<Point> points; // in pixels; relative to position; only relevant if type = Polygon/Polyline
		TileId tile{}; // only relevant if type = Tile
		float x = 0.f; // in pixels
		float y = 0.f; // in pixels
		float width = 0.f; // in pixels
		float height = 0.f; // in pixels
	};

	struct Template {
		std::string path;
		unsigned int object_id = UINT_MAX;
	};

	struct Frame {
		unsigned int duration_ms = 0; // in milliseconds
		unsigned int tile_id = 0; // index into Tileset::tiles[].
	};

	struct Tile {
		std::string class_;
		std::vector<Property> properties;
		std::vector<unsigned int> objects; // indices into Context::objects[]
		std::vector<Frame> animation;
	};

	struct WangColor { // aka "Terrain"
		std::string name;
		std::string class_;
		std::vector<Property> properties;
		// Index into Tileset::tiles[] of the tile representing this wangcolor,
		// or UINT_MAX if no such tile has been chosen.
		unsigned int tile_id = 0;
		float probability = 0.f;
		Color color;
	};

	struct WangTile {
		enum { // Enumerates the edges and corners of the tile.
			TOP = 0,	  // top edge
			TOP_RIGHT,	  // top-right corner
			RIGHT,		  // right edge
			BOTTOM_RIGHT, // bottom-right corner
			BOTTOM,		  // bottom edge
			BOTTOM_LEFT,  // bottom-left corner
			LEFT,		  // left edge
			TOP_LEFT,	  // top-left corner
			COUNT
		};

		// index into Tileset::tiles[].
		unsigned int tile_id = 0;
		// Indices into WangSet::colors[], or UNINT_MAX when the edge/corner is not assigned.
		unsigned int wang_ids[COUNT] = {};
	};

	struct WangSet { // aka "Terrain Set"
		std::string name;
		std::string class_;
		std::vector<Property> properties;
		// Index into Tileset::tiles[] of the tile representing this wangset,
		// or UINT_MAX if no such tile has been chosen.
		unsigned int tile_id = 0;
		std::vector<WangColor> colors; // aka "Terrains"
		std::vector<WangTile> tiles; // only stores the tiles with assigned wangcolors
	};

	struct Tileset {
		std::string path; // empty if embedded in map
		std::string image_path;
		std::string name;
		std::string class_;
		std::vector<Property> properties;
		std::vector<Tile> tiles; // size = tile_count
		std::vector<WangSet> wangsets; // aka "Terrain Sets"
		unsigned int tile_count = 0;
		unsigned int width = 0; // in tiles
		unsigned int height = 0; // in tiles
		unsigned int tile_width = 0; // in pixels
		unsigned int tile_height = 0; // in pixels
		unsigned int spacing = 0; // in pixels
		unsigned int margin = 0; // in pixels
	};

	enum class LayerType {
		Tile,
		Object,
		Image,
		Group,
	};

	struct Layer {
		LayerType type = LayerType::Tile;
		std::string name;
		std::string class_;
		std::vector<Property> properties;
		std::vector<TileId> tiles; // nonempty if type = Tile; in that case size = width * height
		std::vector<unsigned int> objects; // indices into Context::objects[], empty if type != Object 
		unsigned int width = 0; // in tiles
		unsigned int height = 0; // in tiles
		bool visible = true;
	};

	struct Map {
		std::string path;
		std::string class_;
		std::vector<Property> properties;
		std::vector<unsigned int> tilesets; // indices into Context::tilesets[]
		std::vector<Layer> layers;
		unsigned int width = 0; // in tiles
		unsigned int height = 0; // in tiles
		unsigned int tile_width = 0; // in pixels
		unsigned int tile_height = 0; // in pixels
	};

	struct WorldMap {
		unsigned int map_id = UINT_MAX;
		int x = 0; // x-position in world in pixels
		int y = 0; // y-position in world in pixels
		int width = 0; // in pixels
		int height = 0; // in pixels
	};

	struct World {
		std::string path;
		std::vector<WorldMap> maps;
	};

	struct Context {
		bool (*file_load_callback)(std::string_view path, std::string& contents) = nullptr;
		void (*error_message_callback)(std::string_view message) = nullptr;
		std::vector<Object> objects; // stores objects from maps, tiles and templates together
		std::vector<Tileset> tilesets;
		std::vector<Template> templates;
		std::vector<Map> maps;
		std::vector<World> worlds;
	};

	// Returns the tileset ID (an index into Context::tilesets[]), or UINT_MAX on failure.
	unsigned int load_tileset(Context& context, std::string_view path);

	// Returns the object ID (an index into Context::objects[]), or UINT_MAX on failure.
	// May load a tileset as a side effect.
	unsigned int load_template(Context& context, std::string_view path);

	// Returns the map ID (an index into Context::maps[]), or UINT_MAX on failure.
	// May load one or more tilesets and/or templates as a side effect.
	unsigned int load_map(Context& context, std::string_view path);

	// Returns the world ID (an index into Context::worlds[]), or UINT_MAX on failure.
	// Will load maps as a side effect.
	unsigned int load_world(Context& context, std::string_view path);
}
