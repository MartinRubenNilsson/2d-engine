#include "stdafx.h"
#include "ecs_interactions.h"
#include "ecs_physics_queries.h"
#include "ecs_physics_filters.h"

namespace ecs {
	extern entt::registry _registry;

	void set_interaction_event_handler(entt::entity entity, InteractionEventHandler handler) {
		if (handler) {
			_registry.emplace_or_replace<InteractionEventHandler>(entity, handler);
		} else {
			_registry.remove<InteractionEventHandler>(entity);
		}
	}

	InteractionEventHandler get_interaction_event_handler(entt::entity entity) {
		InteractionEventHandler* handler_ptr = _registry.try_get<InteractionEventHandler>(entity);
		if (!handler_ptr) return nullptr;
		return *handler_ptr;
	}

	void interact_with(entt::entity entity, const InteractionEvent& ev) {
		InteractionEventHandler handler = get_interaction_event_handler(entity);
		if (!handler) return;
		handler(entity, ev);
	}

	void interact_with_all_in_box(const Rect2f& box, const InteractionEvent& ev) {
		//shapes::add_box_to_render_queue(box_min, box_max, Color::CYAN, 0.2f);
		for (const OverlapHit& hit : overlap_box(box, ~CC_Player)) {
			interact_with(hit.entity, ev);
		}
	}
}