#include "stdafx.h"
#include "ecs_tiled.h"
#include "tiled.h"
#include "filesystem.h"
#include "console.h"

namespace ecs {
	tiled::Context _tiled_context;

	MapId::operator bool() const {
		return id < _tiled_context.maps.size();
	}

	TilesetId::operator bool() const {
		return id < _tiled_context.tilesets.size();
	}

	TileId::operator bool() const {
		return tileset_id < _tiled_context.tilesets.size() &&
			id < _tiled_context.tilesets[tileset_id].tile_count;
	}

	ObjectId::operator bool() const {
		return id < _tiled_context.objects.size();
	}

	void startup_tiled() {
		_tiled_context.file_load_callback = [](std::string_view path, std::string& contents) {
			return filesystem::read_text_file(path, contents);
		};
		_tiled_context.debug_message_callback = [](std::string_view message) {
			console::log(message);
		};

		// Preload all Tiled assets.
		const std::span<const filesystem::File> files = filesystem::get_all_files_in_directory("assets/tiled");

		// Load tilesets first...
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledTileset) continue;
			tiled::load_tileset_from_file(_tiled_context, file.path);
		}
		// ...then templates...
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledTemplate) continue;
			tiled::load_object_from_file(_tiled_context, file.path);
		}
		// ...and lastly maps.
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledMap) continue;
			tiled::load_map_from_file(_tiled_context, file.path);
		}
	}

	void shutdown_tiled() {
		_tiled_context = {};
	}

	MapId get_map(std::string_view path) {
		for (uint16_t i = 0; i < _tiled_context.maps.size(); ++i) {
			std::string_view tentative_path = _tiled_context.maps[i].path;
			if (tentative_path == path) {
				return { .id = i };
			} else if (filesystem::get_stem(tentative_path) == filesystem::get_stem(path)) {
				return { .id = i };
			}
		}
		return {};
	}

	std::vector<MapId> get_all_maps() {
		std::vector<MapId> maps(_tiled_context.maps.size());
		for (uint16_t i = 0; i < maps.size(); ++i) {
			maps[i] = { .id = i };
		}
		return maps;
	}

	const tiled::Map& _get_map(MapId id) {
		return _tiled_context.maps[id.id];
	}

	std::string_view get_path(MapId map) {
		return _get_map(map).path;
	}

	std::string_view get_name(MapId map) {
		return _get_map(map).name;
	}

	Vec2u get_tile_size(MapId map) {
		const tiled::Map& m = _get_map(map);
		return { m.tile_width, m.tile_height };
	}

	Vec2u get_size_in_tiles(MapId map) {
		const tiled::Map& m = _get_map(map);
		return { m.width, m.height };
	}

	Vec2u get_size_in_pixels(MapId map) {
		const tiled::Map& m = _get_map(map);
		return { m.width * m.tile_width, m.height * m.tile_height };
	}

	bool is_layer_visible(MapId map, unsigned int layer) {
		const tiled::Map& m = _get_map(map);
		if (layer >= m.layers.size()) return false;
		return m.layers[layer].visible;
	}

	TilesetId get_tileset(std::string_view name) {
		if (name.empty()) return {};
		for (uint16_t i = 0; i < _tiled_context.tilesets.size(); ++i) {
			if (_tiled_context.tilesets[i].name == name) {
				return { .id = i };
			}
		}
		return {};
	}

	const tiled::Tileset& _get_tileset(TilesetId tileset) {
		return _tiled_context.tilesets[tileset.id];
	}

	std::string_view get_name(TilesetId tileset) {
		return _get_tileset(tileset).name;
	}

	std::string_view get_class(TilesetId tileset) {
		return _get_tileset(tileset).class_;
	}

	std::string_view get_image_path(TilesetId tileset) {
		return _get_tileset(tileset).image_path;
	}

	Vec2u get_tile_size(TilesetId tileset) {
		const tiled::Tileset& ts = _get_tileset(tileset);
		return { ts.tile_width, ts.tile_height };
	}

	Vec2u get_size_in_tiles(TilesetId tileset) {
		const tiled::Tileset& ts = _get_tileset(tileset);
		return { ts.width, ts.height };
	}

	Vec2u get_size_in_pixels(TilesetId tileset) {
		const tiled::Tileset& ts = _get_tileset(tileset);
		return { ts.width * ts.tile_width, ts.height * ts.tile_height };
	}

	TileId get_tile(TilesetId tileset, unsigned int tile_id) {
		const tiled::Tileset& ts = _get_tileset(tileset);
		if (tile_id >= ts.tile_count) return {};
		return { .id = (uint16_t)tile_id, .tileset_id = tileset.id };
	}

	const tiled::Tile& _get_tile(TileId tile) {
		return _tiled_context.tilesets[tile.tileset_id].tiles[tile.id];
	}

	TilesetId get_tileset(TileId tile) {
		return { tile.tileset_id };
	}

	std::string_view get_class(TileId tile) {
		return _get_tile(tile).class_;
	}

	std::span<const ObjectId> get_objects(TileId tile) {
		const tiled::Tile& t = _get_tile(tile);
		return { (const ObjectId*)t.objects.data(), t.objects.size() };
	}

	Vec2u get_size(TileId tile) {
		const tiled::Tileset& ts = _get_tileset(get_tileset(tile));
		return { ts.tile_width, ts.tile_height };
	}

	TextureRect get_texture_rect(TileId tile) {
		const tiled::Tileset& ts = _get_tileset(get_tileset(tile));
		TextureRect rect{};
		rect.x = (tile.id % ts.width) * (ts.tile_width + ts.spacing) + ts.margin;
		rect.y = (tile.id / ts.width) * (ts.tile_height + ts.spacing) + ts.margin;
		rect.w = ts.tile_width;
		rect.h = ts.tile_height;
		return rect;
	}

	bool animated(TileId tile) {
		return !_get_tile(tile).animation.empty();
	}

	unsigned int get_animation_duration(TileId tile) {
		const auto& animation = _get_tile(tile).animation;
		unsigned int duration_ms = 0; // in milliseconds
		for (const tiled::Frame& frame : animation) {
			duration_ms += frame.duration_ms;
		}
		return duration_ms;
	}

	TileId get_animation_frame(TileId tile, unsigned int time_ms) {
		const auto& animation = _get_tile(tile).animation;
		if (animation.empty())
			return tile;
		for (const tiled::Frame& frame : animation) {
			tile.id = frame.tile_id;
			if (time_ms < frame.duration_ms)
				break;
			time_ms -= frame.duration_ms;
		}
		return tile;
	}

	TileId change(TileId tile, unsigned int id) {
		TileId new_tile = tile;
		new_tile.id = id;
		return new_tile ? new_tile : tile;
	}

	TileId offset(TileId tile, int delta_x, int delta_y) {
		const tiled::Tileset& ts = _tiled_context.tilesets[tile.tileset_id];
		const unsigned int old_x = tile.id % ts.width;
		const unsigned int new_x = (int)old_x + delta_x;
		if (new_x >= ts.width)
			return tile;
		const unsigned int old_y = tile.id / ts.width;
		const unsigned int new_y = (int)old_y + delta_y;
		if (new_y >= ts.height)
			return tile;
		tile.id = (uint16_t)(new_x + new_y * ts.width);
		return tile;
	}

	template <tiled::PropertyType type>
	struct GetPropertyRetVal {
		using Type = std::variant_alternative_t<(size_t)type, tiled::PropertyValue>;
	};

	template <>
	struct GetPropertyRetVal<tiled::PropertyType::String> {
		using Type = std::string_view;
	};

	template <>
	struct GetPropertyRetVal<tiled::PropertyType::File> {
		using Type = std::string_view;
	};

	template <>
	struct GetPropertyRetVal<tiled::PropertyType::Class> {
		using Type = std::string_view;
	};

	const tiled::Object& _get_object(ObjectId obj) {
		return _tiled_context.objects[obj.id];
	}

	template <tiled::PropertyType type>
	GetPropertyRetVal<type>::Type _get_property(ObjectId obj, std::string_view name) {
		const tiled::Object& object = _get_object(obj);
		constexpr size_t index = (size_t)type;
		for (const tiled::Property& prop : object.properties) {
			if (prop.value.index() != index) continue;
			if (prop.name != name) continue;
			return std::get<index>(prop.value);
		}
		return {};
	}

	entt::entity get_entity(ObjectId obj) {
		return (entt::entity)_get_object(obj).uid;
	}

	ObjectType get_type(ObjectId obj) {
		return (ObjectType)_get_object(obj).type;
	}

	std::string_view get_name(ObjectId obj) {
		return _get_object(obj).name;
	}

	std::string_view get_class(ObjectId obj) {
		return _get_object(obj).class_;
	}

	TileId get_tile(ObjectId obj) {
		const tiled::TileGid gid = _get_object(obj).tile;
		return { (uint16_t)gid.id, (uint16_t)gid.tileset_id };
	}

	Vec2f get_position(ObjectId obj) {
		const tiled::Object& o = _get_object(obj);
		return { o.x, o.y };
	}

	Vec2f get_top_left(ObjectId obj) {
		const tiled::Object& o = _get_object(obj);
		Vec2f p = { o.x, o.y };
		if (o.type == tiled::ObjectType::Tile) {
			p.y -= o.height;
		}
		return p;
	}

	Vec2f get_size(ObjectId obj) {
		const tiled::Object& o = _get_object(obj);
		return { o.width, o.height };
	}

	std::span<const Vec2f> get_points(ObjectId obj) {
		const tiled::Object& o = _get_object(obj);
		return { (const Vec2f*)o.points.data(), o.points.size() };
	}

	std::string_view get_string(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::String>(obj, name);
	}

	int get_int(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::Int>(obj, name);
	}

	float get_float(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::Float>(obj, name);
	}

	bool get_bool(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::Bool>(obj, name);
	}

	Color get_color(ObjectId obj, std::string_view name) {
		tiled::Color color = _get_property<tiled::PropertyType::Color>(obj, name);
		return { color.r, color.g, color.b, color.a };
	}

	std::string_view get_file(ObjectId obj, std::string_view name) {
		return _get_property<tiled::PropertyType::File>(obj, name);
	}

	entt::entity get_entity(ObjectId obj, std::string_view name) {
		return (entt::entity)_get_property<tiled::PropertyType::Object>(obj, name);
	}
}
