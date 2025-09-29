#include "stdafx.h"
#include "ecs_tiled.h"
#include "tiled_types.h"

namespace ecs {
	TiledMap::TiledMap(const tiled::Map* map)
		: _map(map)
	{}

	Vector2f TiledMap::get_bottom_right() const {
		return { (float)_map->width  * _map->tile_width,
			     (float)_map->height * _map->tile_height };
	}

	TiledObject::TiledObject(const tiled::Object* obj, const tiled::Map* map)
		: _obj(obj), _map(map)
	{}

	TiledMap TiledObject::get_map() const {
		return { _map };
	}

	Vector2f TiledObject::get_position() const {
		return { _obj->x, _obj->y };
	}

	Vector2f TiledObject::get_top_left() const {
		Vector2f p = get_position();
		if (_obj->type == tiled::ObjectType::Tile) {
			p.y -= _obj->height;
		}
		return p;
	}

	template<tiled::PropertyType type>
	struct GetPropertyRetVal {
		using Type = std::variant_alternative_t<(size_t)type, tiled::PropertyValue>;
	};

	template<>
	struct GetPropertyRetVal<tiled::PropertyType::String> {
		using Type = std::string_view;
	};

	template<>
	struct GetPropertyRetVal<tiled::PropertyType::File> {
		using Type = std::string_view;
	};

	template<>
	struct GetPropertyRetVal<tiled::PropertyType::Class> {
		using Type = std::string_view;
	};

	template<tiled::PropertyType type>
	GetPropertyRetVal<type>::Type _get_property(const tiled::Object& obj, std::string_view name) {
		constexpr size_t index = (size_t)type;
		for (const tiled::Property& prop : obj.properties) {
			if (prop.value.index() != index) continue;
			if (prop.name != name) continue;
			return std::get<index>(prop.value);
		}
		return {};
	}

	std::string_view TiledObject::get_string(std::string_view name) const {
		return _get_property<tiled::PropertyType::String>(*_obj, name);
	}

	int TiledObject::get_int(std::string_view name) const {
		return _get_property<tiled::PropertyType::Int>(*_obj, name);
	}

	float TiledObject::get_float(std::string_view name) const {
		return _get_property<tiled::PropertyType::Float>(*_obj, name);
	}

	bool TiledObject::get_bool(std::string_view name) const {
		return _get_property<tiled::PropertyType::Bool>(*_obj, name);
	}

	Color TiledObject::get_color(std::string_view name) const {
		tiled::Color color = _get_property<tiled::PropertyType::Color>(*_obj, name);
		return { color.r, color.g, color.b, color.a };
	}

	std::string_view TiledObject::get_file(std::string_view name) const {
		return _get_property<tiled::PropertyType::File>(*_obj, name);
	}

	entt::entity TiledObject::get_object(std::string_view name) const {
		return (entt::entity)_get_property<tiled::PropertyType::Object>(*_obj, name);
	}

	extern entt::registry _registry;

	void emplace_tiled_object(entt::entity entity, const TiledObject& obj) {
		_registry.emplace_or_replace<TiledObject>(entity, obj);
	}

	void clear_all_tiled_objects() {
		_registry.clear<TiledObject>();
	}
}
