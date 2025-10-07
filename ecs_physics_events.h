#pragma once

namespace ecs {
	enum class PhysicsEventType {
		SensorBeginTouch,
		SensorEndTouch,
		ContactBeginTouch,
		ContactEndTouch,
	};

	struct PhysicsEvent {
		PhysicsEventType type = PhysicsEventType::SensorBeginTouch;
		b2ShapeId shape = b2_nullShapeId;
		b2ShapeId other_shape = b2_nullShapeId;
		b2BodyId body = b2_nullBodyId;
		b2BodyId other_body = b2_nullBodyId;
		entt::entity entity = entt::null;
		entt::entity other_entity = entt::null;
	};

	using PhysicsEventHandler = void(*)(const PhysicsEvent& ev);

	void set_physics_event_handler(entt::entity entity, PhysicsEventHandler handler);
	PhysicsEventHandler get_physics_event_handler(entt::entity entity); // Returns nullptr if entity has no handler.

	void dispatch_physics_event(PhysicsEvent&& ev);
	void dispatch_physics_event(const b2SensorBeginTouchEvent& b2ev);
	void dispatch_physics_event(const b2SensorEndTouchEvent& b2ev);
	void dispatch_physics_event(const b2ContactBeginTouchEvent& b2ev);
	void dispatch_physics_event(const b2ContactEndTouchEvent& b2ev);
}