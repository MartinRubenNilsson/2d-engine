#pragma once

namespace ecs {
	struct InteractionEvent {
		entt::entity source = entt::null; // The entity that initated the interaction.
		Direction source_dir = Direction::N; // The direction the source is facing.
	};

	using InteractionEventHandler = void(*)(entt::entity entity, const InteractionEvent& ev);

	void set_interaction_event_handler(entt::entity entity, InteractionEventHandler handler);
	InteractionEventHandler get_interaction_event_handler(entt::entity entity);

	// Calls the entity's interaction handler, if one is set.
	void interact_with(entt::entity entity, const InteractionEvent& ev);
	// Calls the interaction handler for all entities that overlap the given box.
	void interact_with_all_in_box(const Rect2f& box, const InteractionEvent& ev);
}