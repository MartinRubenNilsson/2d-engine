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
		b2ShapeId shape_a = b2_nullShapeId;
		b2ShapeId shape_b = b2_nullShapeId;
		b2BodyId body_a = b2_nullBodyId;
		b2BodyId body_b = b2_nullBodyId;
		entt::entity entity_a = entt::null;
		entt::entity entity_b = entt::null;
		Tag tag_a = Tag::None;
		Tag tag_b = Tag::None;
	};

	using PhysicsEventHandler = void(*)(const PhysicsEvent& ev);

	void set_physics_event_handler(entt::entity entity, PhysicsEventHandler handler);
	PhysicsEventHandler get_physics_event_handler(entt::entity entity); // Returns nullptr if entity has no handler.
}