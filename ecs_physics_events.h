#pragma once
#include "ecs_tags.h"

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
		Tag tag = Tag::None;
		Tag other_tag = Tag::None;
	};

	using PhysicsEventHandler = void(*)(const PhysicsEvent& ev);

	void set_physics_event_handler(entt::entity entity, PhysicsEventHandler handler);
	PhysicsEventHandler get_physics_event_handler(entt::entity entity); // Returns nullptr if entity has no handler.
}