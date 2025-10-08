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

	void _dispatch_physics_event(PhysicsEvent&& ev) {
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

	entt::entity _get_entity(void* user_data) {
		return (entt::entity)(uintptr_t)user_data;
	}

	entt::entity _get_entity(b2BodyId body) {
		return _get_entity(b2Body_GetUserData(body));
	}

	void _dispatch_physics_events(const b2SensorBeginTouchEvent& b2ev) {
		PhysicsEvent ev{};
		ev.type = PhysicsEventType::SensorBeginTouch;
		ev.shape = b2ev.sensorShapeId;
		ev.other_shape = b2ev.visitorShapeId;
		ev.body = b2Shape_GetBody(b2ev.sensorShapeId);
		ev.other_body = b2Shape_GetBody(b2ev.visitorShapeId);
		ev.entity = _get_entity(ev.body);
		ev.other_entity = _get_entity(ev.other_body);
		_dispatch_physics_event(std::move(ev));
	}

	void _dispatch_physics_events(const b2SensorEndTouchEvent& b2ev) {
		// PITFALL: Box2D generates this type of events also when two shapes are touching
		// and one of them is destroyed, hence we need to check here if the shapes are valid.
		if (!b2Shape_IsValid(b2ev.sensorShapeId))
			return;
		if (!b2Shape_IsValid(b2ev.visitorShapeId))
			return;
		PhysicsEvent ev{};
		ev.type = PhysicsEventType::SensorEndTouch;
		ev.shape = b2ev.sensorShapeId;
		ev.other_shape = b2ev.visitorShapeId;
		ev.body = b2Shape_GetBody(b2ev.sensorShapeId);
		ev.other_body = b2Shape_GetBody(b2ev.visitorShapeId);
		ev.entity = _get_entity(ev.body);
		ev.other_entity = _get_entity(ev.other_body);
		_dispatch_physics_event(std::move(ev));
	}

	void _dispatch_physics_events(const b2ContactBeginTouchEvent& b2ev) {
		PhysicsEvent ev{};
		ev.type = PhysicsEventType::ContactBeginTouch;
		ev.shape = b2ev.shapeIdA;
		ev.other_shape = b2ev.shapeIdB;
		ev.body = b2Shape_GetBody(b2ev.shapeIdA);
		ev.other_body = b2Shape_GetBody(b2ev.shapeIdB);
		ev.entity = _get_entity(ev.body);
		ev.other_entity = _get_entity(ev.other_body);
		_dispatch_physics_event(std::move(ev));
	}

	void _dispatch_physics_events(const b2ContactEndTouchEvent& b2ev) {
		// PITFALL: Box2D generates this type of events also when two shapes are touching
		// and one of them is destroyed, hence we need to check here if the shapes are valid.
		if (!b2Shape_IsValid(b2ev.shapeIdA))
			return;
		if (!b2Shape_IsValid(b2ev.shapeIdB))
			return;
		PhysicsEvent ev{};
		ev.type = PhysicsEventType::ContactEndTouch;
		ev.shape = b2ev.shapeIdA;
		ev.other_shape = b2ev.shapeIdB;
		ev.body = b2Shape_GetBody(b2ev.shapeIdA);
		ev.other_body = b2Shape_GetBody(b2ev.shapeIdB);
		ev.entity = _get_entity(ev.body);
		ev.other_entity = _get_entity(ev.other_body);
		_dispatch_physics_event(std::move(ev));
	}

	extern b2WorldId _physics_world;

	void _dispatch_sensor_events() {
		const b2SensorEvents events = b2World_GetSensorEvents(_physics_world);
		for (int32_t i = 0; i < events.beginCount; ++i) {
			_dispatch_physics_events(events.beginEvents[i]);
		}
		for (int32_t i = 0; i < events.endCount; ++i) {
			_dispatch_physics_events(events.endEvents[i]);
		}
	}

	void _dispatch_contact_events() {
		const b2ContactEvents events = b2World_GetContactEvents(_physics_world);
		for (int32_t i = 0; i < events.beginCount; ++i) {
			_dispatch_physics_events(events.beginEvents[i]);
		}
		for (int32_t i = 0; i < events.endCount; ++i) {
			_dispatch_physics_events(events.endEvents[i]);
		}
	}
	
	std::vector<BodyMoveEvent> _body_move_events;

	std::span<const BodyMoveEvent> get_body_move_events() {
		return _body_move_events;
	}

	void _dispatch_body_move_events() {
		const b2BodyEvents events = b2World_GetBodyEvents(_physics_world);
		for (int i = 0; i < events.moveCount; ++i) {
			const b2BodyMoveEvent& ev = events.moveEvents[i];
			_body_move_events.emplace_back(ev.bodyId, _get_entity(ev.userData), ev.transform.p);
		}
	}

	void dispatch_physics_events() {
		_dispatch_body_move_events();
		_dispatch_sensor_events();
		_dispatch_contact_events();
	}

	void clear_physics_events() {
		_body_move_events.clear();
	}
}