#pragma once

namespace ecs {
	//TODO: rename "Action" to "Task" maybe?

	enum class AiActionType {
		None, // Do nothing. Runs forever.
		MoveTo, // Move to a certain position. Succeeds when the entity is sufficiently close.
		Flee, // Flee from a certain entity. Succeeds when the entity is sufficiently far away.
		//PlayAnimation, // Play a certain animation. Succeeds when it's finished, fails if it doesn't exist.
	};

	std::string_view to_string(AiActionType type);

	enum class AiActionStatus {
		Running,
		Succeeded,
		Failed,
	};

	struct AiAction {
		AiActionType type = AiActionType::None;
		AiActionStatus status = AiActionStatus::Running;
		float running_time = 0.f;
		std::vector<Vec2i> path; // path of tile positions

		// ACTION-SPECIFIC PARAMETERS

		entt::entity entity = entt::null;
		Vec2f position;
		float speed = 0.f;
		float radius = 0.f;
		float duration = 0.f;
		bool pathfind = false;
	};

	void update_ai_actions(float dt);

	// ACTIONS

	void ai_none(entt::entity entity);
	void ai_move_to(entt::entity entity, const Vec2f& target_position, float speed, float acceptance_radius, bool pathfind = false);
	void ai_flee(entt::entity entity, entt::entity target_entity, float speed, float acceptance_radius);
}
