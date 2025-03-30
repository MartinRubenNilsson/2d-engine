#pragma once
#include "ecs_tags.h"

namespace tiled {
	struct Property;
}

namespace ecs {
	struct Lifetime {
		float time = 0.f;
	};

	void update_lifetimes(float dt);
	void destroy_entities_to_be_destroyed_at_end_of_frame();

	// ENTITY CREATION/DESTRUCTION

	void clear();
	entt::entity create();
	entt::entity create(entt::entity hint);
	entt::entity deep_copy(entt::entity entity);
	void set_lifetime(entt::entity entity, float time);
	void destroy_immediately(entt::entity entity);
	void destroy_at_end_of_frame(entt::entity entity);
	bool valid(entt::entity entity);

	// NAME AND TAG

	void set_name(entt::entity entity, std::string_view name);
	void set_tag(entt::entity entity, Tag tag);
	std::string_view get_name(entt::entity entity);
	Tag get_tag(entt::entity entity);
	entt::entity find_entity_by_name(std::string_view name);
	entt::entity find_entity_by_tag(Tag tag);

	// TILED PROPERTIES

	void set_properties(entt::entity entity, const std::vector<tiled::Property>& properties);
	bool get_properties(entt::entity entity, std::vector<tiled::Property>& properties);
	bool get_string_property(entt::entity entity, std::string_view name, std::string& value);
	bool get_bool_property(entt::entity entity, std::string_view name, bool& value);
	bool get_int_property(entt::entity entity, std::string_view name, int& value);
	bool get_float_property(entt::entity entity, std::string_view name, float& value);
	bool get_object_property(entt::entity entity, std::string_view name, entt::entity& value);
}
