#pragma once
#include "ecs_tags.h"

namespace ecs {
	struct AiEntity {
		entt::entity entity = entt::null;
		Tag tag = Tag::None;
		Vector2f position;
		Vector2f velocity;

		// PROPERTIES
		float p_speed = 0.f;
	};

	struct AiWorld {
		AiEntity player;
	};

	struct AiKnowledge {
		AiEntity me;
		Vector2f initial_position;
		Vector2f initial_velocity;
	};

	void update_ai_knowledge_and_world(float dt);

	const AiWorld& get_ai_world();
	AiKnowledge& emplace_ai_knowledge(entt::entity entity);
}
