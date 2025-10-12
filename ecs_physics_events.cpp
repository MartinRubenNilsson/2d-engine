#include "stdafx.h"
#include "ecs_physics_events.h"
#include "ecs_physics.h"

namespace ecs {
	extern entt::registry _registry;

	void set_touch_event_handler(entt::entity entity, TouchEventHandler handler) {
		if (handler) {
			_registry.emplace_or_replace<TouchEventHandler>(entity, handler);
		} else {
			_registry.remove<TouchEventHandler>(entity);
		}
	}

	TouchEventHandler get_touch_event_handler(entt::entity entity) {
		TouchEventHandler* handler_ptr = _registry.try_get<TouchEventHandler>(entity);
		return handler_ptr ? *handler_ptr : nullptr;
	}

	void _dispatch_physics_event(TouchEvent&& ev) {
		if (TouchEventHandler handler = get_touch_event_handler(ev.entity)) {
			handler(ev);
		}
		if (ev.entity == ev.other_entity)
			return; // Avoid duplicate calls.
		std::swap(ev.shape, ev.other_shape);
		std::swap(ev.body, ev.other_body);
		std::swap(ev.entity, ev.other_entity);
		swap(ev.manifold);
		if (TouchEventHandler handler = get_touch_event_handler(ev.entity)) { // SIC: ev.entity since we swapped
			handler(ev);
		}
	}

	void _dispatch_touch_events(const b2SensorBeginTouchEvent& b2ev) {
		TouchEvent ev{};
		ev.type = TouchEventType::SensorBegin;
		ev.shape = b2ev.sensorShapeId;
		ev.other_shape = b2ev.visitorShapeId;
		ev.body = b2Shape_GetBody(b2ev.sensorShapeId);
		ev.other_body = b2Shape_GetBody(b2ev.visitorShapeId);
		ev.entity = get_entity(ev.body);
		ev.other_entity = get_entity(ev.other_body);
		_dispatch_physics_event(std::move(ev));
	}

	void _dispatch_touch_events(const b2SensorEndTouchEvent& b2ev) {
		TouchEvent ev{};
		ev.type = TouchEventType::SensorEnd;
		// PITFALL: Box2D generates this type of events also when two shapes are touching
		// and one of them is destroyed, hence we need to check here if the shapes are valid.
		ev.shape = b2ev.sensorShapeId;
		if (b2Shape_IsValid(b2ev.sensorShapeId)) {
			ev.body = b2Shape_GetBody(b2ev.sensorShapeId);
			ev.entity = get_entity(ev.body);
		}
		ev.other_shape = b2ev.visitorShapeId;
		if (b2Shape_IsValid(b2ev.visitorShapeId)) {
			ev.other_body = b2Shape_GetBody(b2ev.visitorShapeId);
			ev.other_entity = get_entity(ev.other_body);
		}
		_dispatch_physics_event(std::move(ev));
	}

	void _dispatch_touch_events(const b2ContactBeginTouchEvent& b2ev) {
		TouchEvent ev{};
		ev.type = TouchEventType::ContactBegin;
		ev.shape = b2ev.shapeIdA;
		ev.other_shape = b2ev.shapeIdB;
		ev.body = b2Shape_GetBody(b2ev.shapeIdA);
		ev.other_body = b2Shape_GetBody(b2ev.shapeIdB);
		ev.entity = get_entity(ev.body);
		ev.other_entity = get_entity(ev.other_body);
		ev.manifold = b2ev.manifold;
		_dispatch_physics_event(std::move(ev));
	}

	void _dispatch_touch_events(const b2ContactEndTouchEvent& b2ev) {
		TouchEvent ev{};
		ev.type = TouchEventType::ContactEnd;
		// PITFALL: Box2D generates this type of events also when two shapes are touching
		// and one of them is destroyed, hence we need to check here if the shapes are valid.
		ev.shape = b2ev.shapeIdA;
		if (b2Shape_IsValid(b2ev.shapeIdA)) {
			ev.body = b2Shape_GetBody(b2ev.shapeIdA);
			ev.entity = get_entity(ev.body);
		}
		ev.other_shape = b2ev.shapeIdB;
		if (b2Shape_IsValid(b2ev.shapeIdB)) {
			ev.other_body = b2Shape_GetBody(b2ev.shapeIdB);
			ev.other_entity = get_entity(ev.other_body);
		}
		_dispatch_physics_event(std::move(ev));
	}

	extern b2WorldId _physics_world;

	void _dispatch_sensor_events() {
		const b2SensorEvents events = b2World_GetSensorEvents(_physics_world);
		for (int32_t i = 0; i < events.beginCount; ++i) {
			_dispatch_touch_events(events.beginEvents[i]);
		}
		for (int32_t i = 0; i < events.endCount; ++i) {
			_dispatch_touch_events(events.endEvents[i]);
		}
	}

	void _dispatch_contact_events() {
		const b2ContactEvents events = b2World_GetContactEvents(_physics_world);
		for (int32_t i = 0; i < events.beginCount; ++i) {
			_dispatch_touch_events(events.beginEvents[i]);
		}
		for (int32_t i = 0; i < events.endCount; ++i) {
			_dispatch_touch_events(events.endEvents[i]);
		}
	}
	
	std::vector<BodyMoveEvent> _body_move_events;

	std::span<const BodyMoveEvent> get_body_move_events() {
		return _body_move_events;
	}

	entt::entity _get_entity(void* user_data) {
		return (entt::entity)(uintptr_t)user_data;
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