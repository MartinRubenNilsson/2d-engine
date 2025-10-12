#pragma once

namespace ecs {
	// TODO: InteractionEvent, should contain "entt::entity source"
	using InteractionHandler = void(*)(entt::entity entity);

	void set_interaction_handler(entt::entity entity, InteractionHandler handler);
	InteractionHandler get_interaction_handler(entt::entity entity);

	// Calls the entity's interaction handler, if one is set.
	void interact_with(entt::entity entity);

	// Calls the interaction handler for all entities that intersect the given box.
	void interact_with_all_in_box(const Vec2f& box_min, const Vec2f& box_max);
}