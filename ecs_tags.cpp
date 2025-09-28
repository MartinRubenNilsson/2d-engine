#include "stdafx.h"
#include "ecs_tags.h"
#include "magic_enum/magic_enum_switch.hpp"

using namespace entt::literals;

namespace ecs {
	bool string_to_tag(std::string_view string, Tag& tag) {
		char sanitized_string[64] = {}; // lowercase alphabetic characters only
		for (size_t i = 0, j = 0; i < string.size() && j < std::size(sanitized_string); ++i) {
			if (!isalpha(string[i])) continue;
			sanitized_string[j++] = tolower(string[i]);
		}
		auto value = magic_enum::enum_cast<Tag>(sanitized_string, magic_enum::case_insensitive);
		if (!value.has_value())
			return false;
		tag = value.value();
		return true;
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
}
