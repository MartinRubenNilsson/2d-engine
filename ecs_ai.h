#pragma once

namespace ecs {
	enum class AiType {
		None, // Useful if you want to turn off AI for an entity.
		Slime,

		// Add new ai types here. Examples:
		//Spider,
		//NPC,
		//Boss,
	};

	void update_ai_logic(float dt);

	void emplace_ai(entt::entity entity, AiType type);
}