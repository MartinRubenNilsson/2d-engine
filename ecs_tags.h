#pragma once

namespace ecs {
	enum class Tag {
		None,
		Bounds, // map bounds
		Collider, // static level geometry
		Player,
		Slime,
		PushableBlock,
		Bomb,
		Arrow,
		Pickup,
		Grass,
		Portal,
		BladeTrap,
		Chest,
		Camera,
		AudioSource,
		Count,
	};

	template <Tag tag>
	struct Type {};

	// Returns Tag::None if the string isn't a valid tag.
	Tag to_tag(std::string_view string);
	std::string_view to_string(Tag tag);

	// Emplaces or replaces the Tag component while also emplacing the corresponding
	// Type<new_tag> component (and possibly removing a Type<old_tag> component).
	void set_tag(entt::entity entity, Tag new_tag);
	Tag get_tag(entt::entity entity);

	entt::entity find_entity_with_tag(Tag tag);

	void setup_tags();
}