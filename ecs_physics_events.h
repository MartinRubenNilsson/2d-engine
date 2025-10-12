#pragma once

namespace ecs {
	enum class TouchEventType {
		SensorBegin,
		SensorEnd,
		ContactBegin,
		ContactEnd,
	};

	struct TouchEvent {
		TouchEventType type = TouchEventType::SensorBegin;
		b2ShapeId shape = b2_nullShapeId;
		b2ShapeId other_shape = b2_nullShapeId; // WARNING: may be invalid (but non-null) for End events!
		b2BodyId body = b2_nullBodyId;
		b2BodyId other_body = b2_nullBodyId; // WARNING: may be null for End events!
		entt::entity entity = entt::null;
		entt::entity other_entity = entt::null; // WARNING: may be null for End events!
		b2Manifold manifold{}; // only nonzero for ContactBegin events
	};

	using TouchEventHandler = void(*)(const TouchEvent& ev);

	void set_touch_event_handler(entt::entity entity, TouchEventHandler handler);
	TouchEventHandler get_touch_event_handler(entt::entity entity);

	struct BodyMoveEvent {
		b2BodyId body = b2_nullBodyId;
		entt::entity entity = entt::null;
		Vec2f position; // the new position
	};

	std::span<const BodyMoveEvent> get_body_move_events();

	void clear_physics_events();
	void dispatch_physics_events();
}