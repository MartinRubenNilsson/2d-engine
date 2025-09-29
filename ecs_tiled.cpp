#include "stdafx.h"
#include "ecs_tiled.h"
#include "tiled_types.h"

namespace ecs {
	TiledObject::TiledObject(const tiled::Object& obj)
		: _obj(&obj)
	{}

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

	template <tiled::PropertyType type>
	const auto& _get_property(const tiled::Object& obj, std::string_view name) {
		constexpr size_t index = (size_t)type;
		for (const tiled::Property& prop : obj.properties) {
			if (prop.value.index() != index) continue;
			if (prop.name != name) continue;
			return std::get<index>(prop.value);
		}
		return std::variant_alternative_t<index, tiled::PropertyValue>();
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

	void emplace_tiled_object(entt::entity entity, const tiled::Object& obj) {
		_registry.emplace_or_replace<TiledObject>(entity, obj);
	}

	void clear_all_tiled_objects() {
		_registry.clear<TiledObject>();
	}
}
