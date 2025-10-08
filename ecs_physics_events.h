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
	PhysicsEventHandler get_physics_event_handler(entt::entity entity);

	struct BodyMoveEvent {
		b2BodyId body = b2_nullBodyId;
		entt::entity entity = entt::null;
		Vec2f position; // the new position
	};

	std::span<const BodyMoveEvent> get_body_move_events();

	void clear_physics_events();
	void dispatch_physics_events();
}