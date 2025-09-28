#pragma once

namespace ecs {
	enum class Tag {
		None,
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
	};

	template <Tag tag>
	struct Type {};

	bool string_to_tag(std::string_view string, Tag& tag);
	std::string_view tag_to_string(Tag tag);

	// Emplaces or replaces the Tag component while also emplacing the corresponding
	// Type<new_tag> component (and possibly removing a Type<old_tag> component).
	void set_tag(entt::entity entity, Tag new_tag);
	Tag get_tag(entt::entity entity);
}