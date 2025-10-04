#include "stdafx.h"
#include "ecs_physics_events.h"

namespace ecs {
	extern entt::registry _registry;

	void set_physics_event_handler(entt::entity entity, PhysicsEventHandler handler) {
		if (handler) {
			_registry.emplace_or_replace<PhysicsEventHandler>(entity, handler);
		} else {
			_registry.remove<PhysicsEventHandler>(entity);
		}
	}

	PhysicsEventHandler get_physics_event_handler(entt::entity entity) {
		PhysicsEventHandler* handler_ptr = _registry.try_get<PhysicsEventHandler>(entity);
		return handler_ptr ? *handler_ptr : nullptr;
	}
}