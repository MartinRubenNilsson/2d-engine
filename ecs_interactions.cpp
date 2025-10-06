#include "stdafx.h"
#include "ecs_interactions.h"
#include "ecs_physics_queries.h"
#include "ecs_physics_filters.h"
//#include "shapes.h"

namespace ecs {
	struct InteractionHandlerComponent {
		InteractionHandler handler = nullptr;
	};

	extern entt::registry _registry;

	void set_interaction_handler(entt::entity entity, InteractionHandler handler) {
		if (handler) {
			_registry.emplace_or_replace<InteractionHandlerComponent>(entity, handler);
		} else {
			_registry.remove<InteractionHandlerComponent>(entity);
		}
	}

	InteractionHandler get_interaction_handler(entt::entity entity) {
		InteractionHandlerComponent* component_ptr = _registry.try_get<InteractionHandlerComponent>(entity);
		if (!component_ptr) return nullptr;
		return component_ptr->handler;
	}

	void interact_with(entt::entity entity) {
		InteractionHandler handler = get_interaction_handler(entity);
		if (!handler) return;
		handler(entity);
	}

	void interact_with_all_entities_in_box(const Vec2f& box_min, const Vec2f& box_max) {
		//shapes::add_box_to_render_queue(box_min, box_max, colors::CYAN, 0.2f);
		for (const OverlapHit& hit : overlap_box(box_min, box_max, ~CC_Player)) {
			interact_with(hit.entity);
		}
	}
}