#include "stdafx.h"
#include "ecs_physics.h"
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

	void dispatch_physics_event(PhysicsEvent&& ev) {
		if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity)) {
			handler(ev);
		}
		if (ev.entity == ev.other_entity)
			return; // Avoid duplicate calls.
		std::swap(ev.shape, ev.other_shape);
		std::swap(ev.body, ev.other_body);
		std::swap(ev.entity, ev.other_entity);
		if (PhysicsEventHandler handler = get_physics_event_handler(ev.entity)) { // SIC: ev.entity since we swapped
			handler(ev);
		}
	}

	void dispatch_physics_event(const b2SensorBeginTouchEvent& b2ev) {
		PhysicsEvent ev{};
		ev.type = PhysicsEventType::SensorBeginTouch;
		ev.shape = b2ev.sensorShapeId;
		ev.other_shape = b2ev.visitorShapeId;
		ev.body = b2Shape_GetBody(b2ev.sensorShapeId);
		ev.other_body = b2Shape_GetBody(b2ev.visitorShapeId);
		ev.entity = get_entity(ev.body);
		ev.other_entity = get_entity(ev.other_body);
		dispatch_physics_event(std::move(ev));
	}

	void dispatch_physics_event(const b2SensorEndTouchEvent& b2ev) {
		PhysicsEvent ev{};
		ev.type = PhysicsEventType::SensorEndTouch;
		ev.shape = b2ev.sensorShapeId;
		ev.other_shape = b2ev.visitorShapeId;
		ev.body = b2Shape_GetBody(b2ev.sensorShapeId);
		ev.other_body = b2Shape_GetBody(b2ev.visitorShapeId);
		ev.entity = get_entity(ev.body);
		ev.other_entity = get_entity(ev.other_body);
		dispatch_physics_event(std::move(ev));
	}

	void dispatch_physics_event(const b2ContactBeginTouchEvent& b2_ev) {
		PhysicsEvent ev{};
		ev.type = PhysicsEventType::ContactBeginTouch;
		ev.shape = b2_ev.shapeIdA;
		ev.other_shape = b2_ev.shapeIdB;
		ev.body = b2Shape_GetBody(b2_ev.shapeIdA);
		ev.other_body = b2Shape_GetBody(b2_ev.shapeIdB);
		ev.entity = get_entity(ev.body);
		ev.other_entity = get_entity(ev.other_body);
		dispatch_physics_event(std::move(ev));
	}

	void dispatch_physics_event(const b2ContactEndTouchEvent& b2ev) {
		PhysicsEvent ev{};
		ev.type = PhysicsEventType::ContactEndTouch;
		ev.shape = b2ev.shapeIdA;
		ev.other_shape = b2ev.shapeIdB;
		ev.body = b2Shape_GetBody(b2ev.shapeIdA);
		ev.other_body = b2Shape_GetBody(b2ev.shapeIdB);
		ev.entity = get_entity(ev.body);
		ev.other_entity = get_entity(ev.other_body);
		dispatch_physics_event(std::move(ev));
	}
}