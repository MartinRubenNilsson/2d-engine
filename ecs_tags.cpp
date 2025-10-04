#include "stdafx.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"
#include <magic_enum/magic_enum_switch.hpp>

using namespace entt::literals;

namespace ecs {
	Tag string_to_tag(std::string_view string) {
		char sanitized_string[64] = {}; // lowercase alphabetic characters only
		for (size_t i = 0, j = 0; i < string.size() && j < std::size(sanitized_string); ++i) {
			if (!isalpha(string[i])) continue;
			sanitized_string[j++] = tolower(string[i]);
		}
		auto value = magic_enum::enum_cast<Tag>(sanitized_string, magic_enum::case_insensitive);
		if (!value.has_value())
			return Tag::None;
		return value.value();
	}

	std::string_view tag_to_string(Tag tag) {
		return magic_enum::enum_name(tag);
	}

	extern entt::registry _registry;

	void set_tag(entt::entity entity, Tag new_tag) {
		Tag old_tag = get_tag(entity);
		if (old_tag == new_tag) return;

		// Emplace or replace the Tag component.
		_registry.emplace_or_replace<Tag>(entity, new_tag);

		// Remove the Tag<old_tag> component.
		if (old_tag != Tag::None) {
			magic_enum::enum_switch([entity] (auto old_tag) {
				_registry.erase<Type<old_tag>>(entity);
			}, old_tag);
		}

		// Emplace a Tag<new_tag> component.
		if (new_tag != Tag::None) {
			magic_enum::enum_switch([entity](auto new_tag) {
				_registry.emplace<Type<new_tag>>(entity);
			}, new_tag);
		}
	}

	Tag get_tag(entt::entity entity) {
		if (!_registry.all_of<Tag>(entity))
			return Tag::None;
		return _registry.get<Tag>(entity);
	}

	entt::entity find_entity_by_tag(Tag tag) {
		entt::entity entity = entt::null;
		magic_enum::enum_switch([&entity](auto tag) {
			entity = _registry.view<Type<tag>>().front();
		}, tag);
		return entity;
	}

	void setup_tags() {
		for (auto [entity, object] : _registry.view<ObjectId>().each()) {
			std::string_view class_ = get_class(object);
			if (class_.empty()) continue;
			set_tag(entity, string_to_tag(class_));
		}

		for (auto [entity, tile] : _registry.view<TileId>().each()) {
			std::string_view class_ = get_class(tile);
			if (class_.empty()) continue;
			set_tag(entity, string_to_tag(class_));
		}
	}
}
