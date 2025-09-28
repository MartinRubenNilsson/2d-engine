#include "stdafx.h"
#include "ecs_common.h"
#include "ecs_physics.h"
#include "ecs_player.h"
#include "tiled.h"

namespace ecs {
	struct Name { std::string value; };

	extern entt::registry _registry;
	std::unordered_set<entt::entity> _entities_to_destroy_at_end_of_frame;

	struct Lifetime {
		float time = 0.f;
	};

	void update_lifetimes(float dt) {
		for (auto [entity, lifetime] : _registry.view<Lifetime>().each()) {
			lifetime.time -= dt;
			if (lifetime.time <= 0.f) {
				destroy_at_end_of_frame(entity);
			}
		}
	}

	void destroy_entities_to_be_destroyed_at_end_of_frame() {
		for (entt::entity entity : _entities_to_destroy_at_end_of_frame) {
			if (_registry.valid(entity)) {
				_registry.destroy(entity);
			}
		}
		_entities_to_destroy_at_end_of_frame.clear();
	}

	void clear() {
		_registry.clear();
		_entities_to_destroy_at_end_of_frame.clear();
	}

	entt::entity create() {
		return _registry.create();
	}

	entt::entity create(entt::entity hint) {
		return _registry.create(hint);
	}

	entt::entity deep_copy(entt::entity entity) {
		entt::entity copied_entity = _registry.create();
		for (auto [name, storage] : _registry.storage()) {
			if (!storage.contains(entity)) continue;
			if (storage.type() == entt::type_id<b2BodyId>()) {
				deep_copy_and_emplace_body(copied_entity, *(b2BodyId*)storage.value(entity));
			} else if (storage.type() == entt::type_id<Player>()) {
				// TODO: deep copy player, since it holds a held item entity
				storage.push(copied_entity, storage.value(entity));
			} else {
				storage.push(copied_entity, storage.value(entity));
			}
		}
		return copied_entity;
	}

	void set_lifetime(entt::entity entity, float time) {
		_registry.emplace_or_replace<Lifetime>(entity, time);
	}

	void destroy_immediately(entt::entity entity) {
		if (_registry.valid(entity)) {
			_registry.destroy(entity);
		}
	}

	void destroy_at_end_of_frame(entt::entity entity) {
		if (_registry.valid(entity)) {
			_entities_to_destroy_at_end_of_frame.insert(entity);
		}
	}

	bool valid(entt::entity entity) {
		return _registry.valid(entity);
	}

	void set_name(entt::entity entity, std::string_view name) {
		_registry.emplace_or_replace<Name>(entity, std::string(name));
	}

	std::string_view get_name(entt::entity entity) {
		const Name* name = _registry.try_get<const Name>(entity);
		if (!name) return "";
		return name->value;
	}

	entt::entity find_entity_by_name(std::string_view name) {
		if (name.empty()) return entt::null;
		for (auto [entity, other_name] : _registry.view<const Name>().each()) {
			if (other_name.value == name) return entity;
		}
		return entt::null;
	}

	entt::entity find_entity_by_tag(Tag tag) {
		for (auto [entity, other_tag] : _registry.view<Tag>().each()) {
			if (other_tag == tag) return entity;
		}
		return entt::null;
	}

	void set_properties(entt::entity entity, const TiledProperties& properties) {
		_registry.emplace_or_replace<TiledProperties>(entity, properties);
	}

	bool get_properties(entt::entity entity, TiledProperties& properties) {
		if (!_registry.all_of<TiledProperties>(entity)) return false;
		properties = _registry.get<TiledProperties>(entity);
		return true;
	}

	bool get_string_property(entt::entity entity, std::string_view name, std::string& value) {
		const TiledProperties* properties = _registry.try_get<const TiledProperties>(entity);
		if (!properties) return false;
		return get<tiled::PropertyType::String>(*properties, name, value);
	}

	bool get_bool_property(entt::entity entity, std::string_view name, bool& value) {
		const TiledProperties* properties = _registry.try_get<const TiledProperties>(entity);
		if (!properties) return false;
		return get<tiled::PropertyType::Bool>(*properties, name, value);
	}

	bool get_int_property(entt::entity entity, std::string_view name, int& value) {
		const TiledProperties* properties = _registry.try_get<const TiledProperties>(entity);
		if (!properties) return false;
		return get<tiled::PropertyType::Int>(*properties, name, value);
	}

	bool get_float_property(entt::entity entity, std::string_view name, float& value) {
		const TiledProperties* properties = _registry.try_get<const TiledProperties>(entity);
		if (!properties) return false;
		return get<tiled::PropertyType::Float>(*properties, name, value);
	}

	bool get_object_property(entt::entity entity, std::string_view name, entt::entity& value) {
		const TiledProperties* properties = _registry.try_get<const TiledProperties>(entity);
		if (!properties) return false;
		return get<tiled::PropertyType::Object>(*properties, name, (unsigned int&)value);
	}
}
