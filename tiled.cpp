#include "tiled.h"
#include <pugixml.hpp>
#include <zlib.h>
#include <filesystem> // TODO: remove, it is slow to compile!
#include <span>
#include <cstdlib>

namespace tiled {

	// Loads a color from a string in the format "#RRGGBB" or "#AARRGGBB"
	Color _load_color(const char* str) {
		Color color{ .r = 0, .g = 0, .b = 0, .a = 0xFF };
		if (!str) {
			return color;
		}
		if (*str == '#') {
			++str; // skip leading '#'
		}
		const size_t len = strlen(str);
		if (len < 2) return color;
		const unsigned long color_hex = strtoul(str, nullptr, 16);
		color.b = color_hex & 0xFF;
		if (len < 4) return color;
		color.g = (color_hex >> 8) & 0xFF;
		if (len < 6) return color;
		color.r = (color_hex >> 16) & 0xFF;
		if (len < 8) return color;
		color.a = (color_hex >> 24) & 0xFF;
		return color;
	}

	void _load_property(const pugi::xml_node& node, Property& prop) {
		prop.name = node.attribute("name").as_string();
		const std::string_view type = node.attribute("type").as_string("string"); // default type is string
		const pugi::xml_attribute value = node.attribute("value");
		if (type == "string") {
			prop.value.emplace<(size_t)PropertyType::String>(value.as_string());
		} else if (type == "int") {
			prop.value.emplace<(size_t)PropertyType::Int>(value.as_int());
		} else if (type == "float") {
			prop.value.emplace<(size_t)PropertyType::Float>(value.as_float());
		} else if (type == "bool") {
			prop.value.emplace<(size_t)PropertyType::Bool>(value.as_bool());
		} else if (type == "color") {
			prop.value.emplace<(size_t)PropertyType::Color>(_load_color(value.as_string()));
		} else if (type == "file") {
			prop.value.emplace<(size_t)PropertyType::File>(value.as_string());
		} else if (type == "object") {
			prop.value.emplace<(size_t)PropertyType::Object>(value.as_uint());
		} else if (type == "class") {
			// TODO
		} else {
			// Unknown property type
		}
	}

	void _load_properties(const pugi::xml_node& node, std::vector<Property>& properties) {
		for (pugi::xml_node prop_node : node.child("properties").children("property")) {
			_load_property(prop_node, properties.emplace_back());
		}
	}

	void _load_points(const pugi::xml_node& node, std::vector<Point>& points) {
		// Example: <polygon points="0,0 0,16 16,16"/>
		const char* str = node.attribute("points").as_string();
		if (!str) return;
		while (*str) { // loop until end of string
			Point& point = points.emplace_back();
			int num_chars_read = 0;
			if (sscanf_s(str, "%f,%f%n", &point.x, &point.y, &num_chars_read) != 2) {
				break; // error reading point
			}
			str += num_chars_read;
			if (*str == ' ') {
				++str; // skip space
			}
		}
	}

	// PITFALL: Tiled assigns GIDs to tiles by (conceptually) concatenating all tilesets end-to-end
	// in some arbitrary order. Then, each tile can be assigned a unique index in this concatenated
	// "mega-tileset". The indexing starts at 1, so in Tiled a GID of 0 means that the tile is empty.
	//
	// This means that Tiled GIDs are always with reference to some map, which is not useful for our
	// purposes. Hence we "resolve" these GIDs to a pair of IDs, one for the tileset and one for the
	// tile in that tileset. This is done by iterating through all the tilesets used in the map, and,
	// if the tileset has a range of GIDs which contains the tile GID, then we replace the GID with
	// the tile ID and the tile's local ID in that tileset.
	//
	// We use TileGid::ids to store both the original Tile GID (before resolving) and our pair of IDs
	// (after resolving), so watch out!

	struct TilesetWithGids {
		unsigned int id = UINT_MAX; // index into Context::tilesets[]
		unsigned int first_gid = 0; // The Tiled GID of the first tile in the tileset.
		unsigned int last_gid = 0; // The Tiled GID of the last tile in the tileset.
	};

	bool _resolve_gid(TileGid& tile, const TilesetWithGids& tileset) {
		if (tile.ids < tileset.first_gid || tileset.last_gid < tile.ids)
			return false;
		tile.id = tile.ids - tileset.first_gid;
		tile.tileset_id = tileset.id;
		return true;
	}

	bool _resolve_gid(TileGid& tile, std::span<const TilesetWithGids> tilesets) {
		for (const TilesetWithGids&tileset : tilesets) {
			if (_resolve_gid(tile, tileset)) {
				return true;
			}
		}
		tile = {};
		return false;
	}

	void _load_object(const pugi::xml_node& node, Object& object, std::span<const TilesetWithGids> tilesets) {
		// TODO: this doesn't actually override properties for template objects!
		// there will be duplicated properties!!!
		_load_properties(node, object.properties);
		if (pugi::xml_attribute id = node.attribute("id")) {
			object.uid = id.as_uint();
		}
		if (pugi::xml_attribute name = node.attribute("name")) {
			object.name = name.as_string();
		}
		if (pugi::xml_attribute type = node.attribute("type")) { // SIC: "type", not "class"
			object.class_ = type.as_string();
		}
		if (pugi::xml_attribute x = node.attribute("x")) {
			object.x = x.as_float();
		}
		if (pugi::xml_attribute y = node.attribute("y")) {
			object.y = y.as_float();
		}
		if (pugi::xml_attribute width = node.attribute("width")) {
			object.width = width.as_float();
		}
		if (pugi::xml_attribute height = node.attribute("height")) {
			object.height = height.as_float();
		}
		if (node.child("ellipse")) {
			object.type = ObjectType::Ellipse;
		} else if (node.child("point")) {
			object.type = ObjectType::Point;
		} else if (pugi::xml_node polygon = node.child("polygon")) {
			object.type = ObjectType::Polygon;
			_load_points(polygon, object.points);
		} else if (pugi::xml_node polyline = node.child("polyline")) {
			object.type = ObjectType::Polyline;
			_load_points(polyline, object.points);
		} else if (pugi::xml_node text = node.child("text")) {
			object.type = ObjectType::Text;
			//TODO
		}
		if (pugi::xml_attribute gid = node.attribute("gid")) {
			object.type = ObjectType::Tile;
			object.tile.value = gid.as_uint();
			_resolve_gid(object.tile, tilesets);
		}
	}

	std::string _get_normalized_path(std::string_view path) {
		return std::filesystem::path(path).lexically_normal().string();
	}

	std::string _get_parent_path(std::string_view path) {
		return std::filesystem::path(path).parent_path().string();
	}

	std::string _get_path_stem(std::string_view path) {
		return std::filesystem::path(path).stem().string();
	}

	unsigned int load_tileset_from_file(Context& context, std::string_view path) {
		std::string normalized_path = _get_normalized_path(path);

		// Check if the tileset is already loaded
		for (unsigned int id = 0; id < context.tilesets.size(); ++id) {
			if (context.tilesets[id].path == normalized_path) {
				return id;
			}
		}

		if (!context.file_load_callback) {
			if (context.debug_message_callback) {
				context.debug_message_callback("File load callback is not set: " + normalized_path);
			}
			return UINT_MAX;
		}

		std::string file_contents;
		if (!context.file_load_callback(normalized_path, file_contents)) {
			if (context.debug_message_callback) {
				context.debug_message_callback("Failed to load tileset file: " + normalized_path);
			}
			return UINT_MAX;
		}

		pugi::xml_document doc;
		if (!doc.load_buffer_inplace(file_contents.data(), file_contents.size())) {
			if (context.debug_message_callback) {
				context.debug_message_callback("Failed to parse tileset file: " + normalized_path);
			}
			return UINT_MAX;
		}
		const pugi::xml_node tileset_node = doc.child("tileset");

		Tileset tileset{};
		tileset.path = std::move(normalized_path);
		tileset.name = tileset_node.attribute("name").as_string();
		tileset.class_ = tileset_node.attribute("class").as_string();
		tileset.tile_width = tileset_node.attribute("tilewidth").as_uint();
		tileset.tile_height = tileset_node.attribute("tileheight").as_uint();
		tileset.tile_count = tileset_node.attribute("tilecount").as_uint();
		tileset.width = tileset_node.attribute("columns").as_uint();
		tileset.height = tileset.tile_count / tileset.width;
		tileset.spacing = tileset_node.attribute("spacing").as_uint();
		tileset.margin = tileset_node.attribute("margin").as_uint();
		tileset.image_path = _get_parent_path(tileset.path);
		tileset.image_path += '/';
		tileset.image_path += tileset_node.child("image").attribute("source").as_string();
		tileset.image_path = _get_normalized_path(tileset.image_path);
		_load_properties(tileset_node, tileset.properties);
		tileset.tiles.resize(tileset.tile_count);

		// Load tiles
		for (pugi::xml_node tile_node : tileset_node.children("tile")) {
			const unsigned int tile_id = tile_node.attribute("id").as_uint();
			if (tile_id >= tileset.tile_count) {
				if (context.debug_message_callback) {
					context.debug_message_callback("Invalid tile ID in tileset: " + std::to_string(tile_id));
				}
				continue;
			}
			Tile& tile = tileset.tiles[tile_id];
			tile.class_ = tile_node.attribute("type").as_string(); // SIC: "type", not "class"
			_load_properties(tile_node, tile.properties);
			for (pugi::xml_node object_node : tile_node.child("objectgroup").children("object")) {
				tile.objects.push_back((unsigned int)context.objects.size());
				_load_object(object_node, context.objects.emplace_back(), {});
			}
			for (pugi::xml_node frame_node : tile_node.child("animation").children("frame")) {
				Frame& frame = tile.animation.emplace_back();
				frame.duration_ms = frame_node.attribute("duration").as_uint();
				frame.tile_id = frame_node.attribute("tileid").as_uint();
			}
		}

		// Load Wangsets
		for (pugi::xml_node wangset_node : tileset_node.child("wangsets").children("wangset")) {
			WangSet& wangset = tileset.wangsets.emplace_back();
			wangset.name = wangset_node.attribute("name").as_string();
			wangset.class_ = wangset_node.attribute("class").as_string();
			_load_properties(wangset_node, wangset.properties);
			// PITFALL: The "tile" attribute is stored as an int, so if we try to use as_uint(),
			// we find that it returns 0 for negative values. This is a problem because -1 is used
			// to indicate that no tile is set. Hence we use as_int() and cast to unsigned int.
			wangset.tile_id = (unsigned int)wangset_node.attribute("tile").as_int(); // UINT_MAX in case of no tile
			for (pugi::xml_node wangcolor_node : wangset_node.children("wangcolor")) {
				WangColor& wangcolor = wangset.colors.emplace_back();
				wangcolor.name = wangcolor_node.attribute("name").as_string();
				wangcolor.class_ = wangcolor_node.attribute("class").as_string();
				_load_properties(wangcolor_node, wangcolor.properties);
				// PITFALL: The "tile" attribute is stored as an int, so if we try to use as_uint(),
				// we find that it returns 0 for negative values. This is a problem because -1 is used
				// to indicate that no tile is set. Hence we use as_int() and cast to unsigned int.
				wangcolor.tile_id = (unsigned int)wangcolor_node.attribute("tile").as_int(); // UINT_MAX in case of no tile
				wangcolor.probability = wangcolor_node.attribute("probability").as_float();
				wangcolor.color = _load_color(wangcolor_node.attribute("color").as_string());
			}
			for (pugi::xml_node wangtile_node : wangset_node.children("wangtile")) {
				WangTile& wangtile = wangset.tiles.emplace_back();
				wangtile.tile_id = wangtile_node.attribute("tileid").as_uint();
				memset(wangtile.wang_ids, 0xFF, sizeof(wangtile.wang_ids));
				// wangid_str is a comma-separated list of 8 integer tile IDs, one for each corner/edge of the tile.
				const char* wangid_str = wangtile_node.attribute("wangid").as_string();
				if (!wangid_str) continue;
				for (int i = 0; i < WangTile::COUNT && *wangid_str; ++i) {
					// wang_id = 0 means unset, wang_id = 1 means first color, etc.
					// Conveniently, wang_id = 0 is also the return value of strtoul() when parsing fails.
					char* wangid_str_end;
					if (unsigned int wang_id = strtoul(wangid_str, &wangid_str_end, 10)) {
						wangtile.wang_ids[i] = wang_id - 1;
					}
					wangid_str = wangid_str_end;
					while (*wangid_str && !isdigit(*wangid_str)) {
						++wangid_str;
					}
				}
			}
		}

		const unsigned int id = (unsigned int)context.tilesets.size();
		context.tilesets.emplace_back(std::move(tileset));
		return id;
	}

	unsigned int load_object_from_file(Context& context, std::string_view path) {
		std::string normalized_path = _get_normalized_path(path);

		// Check if the object is already loaded
		for (unsigned int id = 0; id < context.objects.size(); ++id) {
			if (context.objects[id].path == normalized_path) {
				return id;
			}
		}

		if (!context.file_load_callback) {
			if (context.debug_message_callback) {
				context.debug_message_callback("File load callback is not set: " + normalized_path);
			}
			return UINT_MAX;
		}

		std::string file_contents;
		if (!context.file_load_callback(normalized_path, file_contents)) {
			if (context.debug_message_callback) {
				context.debug_message_callback("Failed to load Tiled template: " + normalized_path);
			}
			return UINT_MAX;
		}

		pugi::xml_document doc;
		if (!doc.load_buffer_inplace(file_contents.data(), file_contents.size())) {
			if (context.debug_message_callback) {
				context.debug_message_callback("Failed to parse Tiled template: " + normalized_path);
			}
			return UINT_MAX;
		}

		const pugi::xml_node template_node = doc.child("template");

		// Load tileset (if there is one)
		TilesetWithGids tileset{};
		if (pugi::xml_node tileset_node = template_node.child("tileset")) {
			const pugi::xml_attribute source_attribute = tileset_node.attribute("source");
			if (!source_attribute) {
				if (context.debug_message_callback) {
					context.debug_message_callback("Tileset source attribute is missing: " + normalized_path);
				}
			} else {
				std::string tileset_path = _get_parent_path(normalized_path);
				tileset_path += '/';
				tileset_path += source_attribute.as_string();
				tileset_path = _get_normalized_path(tileset_path);
				tileset.id = load_tileset_from_file(context, tileset_path);
				if (tileset.id < context.tilesets.size()) {
					// NOTE: first_gid should always be 1 for template objects.
					tileset.first_gid = tileset_node.attribute("firstgid").as_uint();;
					tileset.last_gid = tileset.first_gid + context.tilesets[tileset.id].tile_count - 1;
				}
			}
		}

		const pugi::xml_node object_node = template_node.child("object");

		Object object{};
		object.path = std::move(normalized_path);
		_load_object(object_node, object, std::span(&tileset, 1));

		const unsigned int id = (unsigned int)context.objects.size();
		context.objects.emplace_back(std::move(object));
		return id;
	}

	bool _string_to_layer_type(std::string_view str, LayerType& type) {
		if (str == "layer") {
			type = LayerType::Tile;
		} else if (str == "objectgroup") {
			type = LayerType::Object;
		} else if (str == "imagelayer") {
			type = LayerType::Image;
		} else if (str == "group") {
			type = LayerType::Group;
		} else {
			return false;
		}
		return true;
	}

	// Maps each ASCII character to its corresponding 6-bit Base64 value,
	// or 0 if the character is not part of the Base64 alphabet.
	const unsigned char _BASE64_DECODING_TABLE[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* + */ 62, 0, 0, 0, /* / */ 63, /* 0-9 */ 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, /* = */ 0, 0, 0, 0, 0,
		/* A-Z */ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0,
		/* a-z */ 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

	void _base64_decode(unsigned char* dest, size_t dest_len, const unsigned char* source, size_t source_len) {
		if (!dest || !source) return;
		size_t i = 0;
		size_t j = 0;
		while (i + 3 < source_len && j + 2 < dest_len) {
			unsigned char a = _BASE64_DECODING_TABLE[source[i++]];
			unsigned char b = _BASE64_DECODING_TABLE[source[i++]];
			unsigned char c = _BASE64_DECODING_TABLE[source[i++]];
			unsigned char d = _BASE64_DECODING_TABLE[source[i++]];
			dest[j++] = (a << 2) | (b >> 4);
			dest[j++] = (b << 4) | (c >> 2);
			dest[j++] = (c << 6) | d;
		}
	}

	void _load_layer_recursive(
		Context& context,
		Map& map,
		const std::vector<TilesetWithGids>& tilesets_with_gids,
		const pugi::xml_node& node
	) {
		// PITFALL: node may be of type <tileset>, and we are only interested in layers,
		// hence we have an early return if we fail to convert the node name to a LayerType.

		LayerType type;
		if (!_string_to_layer_type(node.name(), type)) return;

		Layer& layer = map.layers.emplace_back();
		layer.type = type;
		layer.name = node.attribute("name").as_string();
		layer.class_ = node.attribute("class").as_string();
		layer.width = node.attribute("width").as_uint();
		layer.height = node.attribute("height").as_uint();
		layer.visible = node.attribute("visible").as_bool(true); // default is true
		_load_properties(node, layer.properties);

		switch (layer.type) {
		case LayerType::Tile: {
			const pugi::xml_node data_node = node.child("data");
			const std::string_view encoding = data_node.attribute("encoding").as_string();
			layer.tiles.resize(layer.width * layer.height);
			if (encoding == "csv") {
				const char* csv_str = data_node.text().as_string();
				if (!csv_str) break; // error reading CSV string
				int t = 0; // current tile index
				while (*csv_str) {
					if (t >= layer.tiles.size()) break; // too many tiles
					while (!isdigit(*csv_str)) {
						++csv_str; // skip non-digit characters
					}
					if (!*csv_str) break; // end of string
					// Read a tile GID from the CSV string
					unsigned int gid = 0;
					int num_chars_read = 0;
					if (sscanf_s(csv_str, "%u%n", &gid, &num_chars_read) != 1) {
						break; // error reading tile GID
					}
					layer.tiles[t++].value = gid;
					csv_str += num_chars_read;
				}
			} else if (encoding == "base64") {

				std::vector<unsigned char> compressed_data;

				// DECODE BASE64
				{
					std::string_view base64_string = data_node.text().as_string();
					// Skip leading whitespace
					while (!base64_string.empty() && isspace(base64_string.front())) {
						base64_string.remove_prefix(1);
					}
					// Skip trailing whitespace
					while (!base64_string.empty() && isspace(base64_string.back())) {
						base64_string.remove_suffix(1);
					}
					// Base64 string length must be a multiple of 4
					if (base64_string.size() % 4 != 0) {
						if (context.debug_message_callback) {
							context.debug_message_callback(
								"Invalid Base64 string length: " + std::to_string(base64_string.size()) + "\n"
								"  Map: " + map.path + "\n"
								"  Layer: " + layer.name);
						}
						break;
					}
					compressed_data.resize((base64_string.size() / 4) * 3);
					_base64_decode(compressed_data.data(), compressed_data.size(),
						(const unsigned char*)base64_string.data(), base64_string.size());
				}

				// DECOMPRESS

				const std::string_view compression = data_node.attribute("compression").as_string();
				if (compression == "zlib") {
					uLongf data_size = (uLongf)(layer.tiles.size() * sizeof(unsigned int));
					uncompress((Bytef*)layer.tiles.data(), &data_size,
						(Bytef*)compressed_data.data(), (uLongf)compressed_data.size());
				} else if (compression.empty()) { // No compression
					// We may have up to 3 bytes of padding at the end of compressed_data
					// as a result of the Base64 decoding process; these can be discarded.
					if (compressed_data.size() < layer.tiles.size() * sizeof(unsigned int)) {
						if (context.debug_message_callback) {
							context.debug_message_callback(
								"Base64-decoded data is too small: " + std::to_string(compressed_data.size()) + "\n"
								"  Map: " + map.path + "\n"
								"  Layer: " + layer.name);
						}
						break;
					}
					memcpy(layer.tiles.data(), compressed_data.data(), layer.tiles.size() * sizeof(unsigned int));
				} else {
					// Unknown compression type
					if (context.debug_message_callback) {
						context.debug_message_callback(
							"Unknown Tiled map tile layer compression: " + std::string(compression) + "\n"
							"  Map: " + map.path + "\n"
							"  Layer: " + layer.name);
					}
				}

			} else {
				// Unknown encoding type
				if (context.debug_message_callback) {
					context.debug_message_callback(
						"Unknown Tiled map tile layer encoding: " + std::string(encoding) + "\n"
						"  Map: " + map.path + "\n"
						"  Layer: " + layer.name);
				}
			}
			// Resolve GIDs. This converts Tiled GIDs (which are local to this map) into a pair
			// of IDs which can be used to reference the tile independently of this map.
			for (TileGid& tile : layer.tiles) {
				_resolve_gid(tile, tilesets_with_gids);
			}
		} break;
		case LayerType::Object: {
			for (pugi::xml_node object_node : node.children("object")) {
				layer.objects.push_back((unsigned int)context.objects.size());
				Object& object = context.objects.emplace_back();
				// If the object is connected to a template, we need to load and apply it first.
				if (pugi::xml_attribute template_attribute = object_node.attribute("template")) {
					std::string template_path = _get_parent_path(map.path);
					template_path += '/';
					template_path += template_attribute.as_string();
					template_path = _get_normalized_path(template_path);
					const unsigned int object_id = load_object_from_file(context, template_path);
					if (object_id < context.objects.size()) {
						object = context.objects[object_id];
					}
				}
				// By loading the object after copying the template, we can override properties.
				_load_object(object_node, object, tilesets_with_gids);
			}
		} break;
		case LayerType::Image: {
			// TODO
		} break;
		case LayerType::Group: {
			for (pugi::xml_node child_node : node.children()) {
				_load_layer_recursive(context, map, tilesets_with_gids, child_node);
			}
		} break;
		}
	}

	unsigned int load_map_from_file(Context& context, std::string_view path) {
		std::string normalized_path = _get_normalized_path(path);

		// Check if the map is already loaded
		for (unsigned int id = 0; id < context.maps.size(); ++id) {
			if (context.maps[id].path == normalized_path) {
				return id;
			}
		}

		if (!context.file_load_callback) {
			if (context.debug_message_callback) {
				context.debug_message_callback("File load callback is not set: " + normalized_path);
			}
			return UINT_MAX;
		}

		std::string file_contents;
		if (!context.file_load_callback(normalized_path, file_contents)) {
			if (context.debug_message_callback) {
				context.debug_message_callback("Failed to load Tiled map: " + normalized_path);
			}
			return UINT_MAX;
		}

		pugi::xml_document doc;
		if (!doc.load_buffer_inplace(file_contents.data(), file_contents.size())) {
			if (context.debug_message_callback) {
				context.debug_message_callback("Failed to parse Tiled map: " + normalized_path);
			}
			return UINT_MAX;
		}
		pugi::xml_node map_node = doc.child("map");

		Map map{};
		map.path = std::move(normalized_path);
		map.name = _get_path_stem(map.path);
		map.class_ = map_node.attribute("class").as_string();
		map.width = map_node.attribute("width").as_uint();
		map.height = map_node.attribute("height").as_uint();
		map.tile_width = map_node.attribute("tilewidth").as_uint();
		map.tile_height = map_node.attribute("tileheight").as_uint();
		_load_properties(map_node, map.properties);

		// Load tilesets
		std::vector<TilesetWithGids> tilesets;
		for (pugi::xml_node tileset_node : map_node.children("tileset")) {
			pugi::xml_attribute source_attribute = tileset_node.attribute("source");
			// TODO: handle embedded tilesets
			if (!source_attribute) {
				if (context.debug_message_callback) {
					context.debug_message_callback("Embedded tilesets are not supported: " + map.path);
				}
				continue;
			}
			std::string tileset_path = _get_parent_path(map.path);
			tileset_path += '/';
			tileset_path += source_attribute.as_string();
			tileset_path = _get_normalized_path(tileset_path);
			const unsigned int tileset_id = load_tileset_from_file(context, tileset_path);
			if (tileset_id >= context.tilesets.size())
				continue;
			map.tilesets.push_back(tileset_id);
			const unsigned int first_gid = tileset_node.attribute("firstgid").as_uint();
			const unsigned int last_gid = first_gid + context.tilesets[tileset_id].tile_count - 1;
			tilesets.emplace_back(tileset_id, first_gid, last_gid);
		}

		// Load layers
		for (pugi::xml_node child_node : map_node.children()) {
			_load_layer_recursive(context, map, tilesets, child_node);
		}

		const unsigned int id = (unsigned int)context.maps.size();
		context.maps.emplace_back(std::move(map));
		return id;
	}
}