#include "stdafx.h"

// IMPORTANT:
// Do not include or use AiKnowledge, AiWorld or AiType in this file.
// The AiActions need to be decoupled from these, since we want to be
// able to trigger AiActions from other systems as well.
// Hence they need to run independently from the rest of the AI system.

#include "ecs_ai_action.h"
#include "ecs_physics.h"
#include "random.h"
#include "map_grid.h"

namespace ecs {
	extern entt::registry _registry;
	float _ai_action_time = 0.f;

	std::string_view to_string(AiActionType type) {
		return magic_enum::enum_name(type);
	}

	void update_ai_actions(float dt) {
		_ai_action_time += dt;

		for (auto [entity, action] : _registry.view<AiAction>().each()) {
			if (action.status == AiActionStatus::Running)
				action.running_time += dt;
		}

		for (auto [entity, action, body] : _registry.view<AiAction, b2BodyId>().each()) {
			if (action.status != AiActionStatus::Running) continue;

			//const Vec2f my_pos = b2Body_GetPosition(body);
			const Vec2f my_pos = b2Body_GetWorldCenterOfMass(body);
			const Vec2f my_old_dir = normalize(b2Body_GetLinearVelocity(body));
			Vec2f my_new_dir;

			switch (action.type) {
				case AiActionType::None: {
					// Do nothing.
				} break;
				case AiActionType::MoveTo: {
					Vec2f to_target = action.position - my_pos;
					float dist = length(to_target);
					if (dist <= action.radius) {
						action.status = AiActionStatus::Succeeded;
					} else {
						my_new_dir = to_target / dist;
					}
				} break;
				case AiActionType::Flee: {
					b2BodyId danger_body = get_body(action.entity);
					if (B2_IS_NULL(danger_body)) {
						action.status = AiActionStatus::Failed;
						break;
					}
					Vec2f danger_pos = b2Body_GetPosition(danger_body);
					Vec2f to_danger = danger_pos - my_pos;
					float dist = length(to_danger);
					if (dist >= action.radius) {
						action.status = AiActionStatus::Succeeded;
					} else {
						my_new_dir = -(to_danger / dist); // Note the minus sign.
					}
				} break;
			}

			b2Body_SetLinearVelocity(body, action.speed * my_new_dir);
		}
	}

	void _replace_ai_action(entt::entity entity, const AiAction& action) {
		_registry.emplace_or_replace<AiAction>(entity, action);
	}

	void ai_none(entt::entity entity) {
		AiAction action{};
		action.type = AiActionType::None;
		_replace_ai_action(entity, action);
	}

	void ai_move_to(entt::entity entity, const Vec2f& target_position, float speed, float acceptance_radius, bool pathfind) {
		AiAction action{};
		action.type = AiActionType::MoveTo;
		action.position = target_position;
		action.speed = speed;
		action.radius = acceptance_radius;
		action.pathfind = pathfind;
		_replace_ai_action(entity, action);
	}

	void ai_flee(entt::entity entity, entt::entity target_entity, float speed, float acceptance_radius) {
		AiAction action{};
		action.type = AiActionType::Flee;
		action.entity = target_entity;
		action.speed = speed;
		action.radius = acceptance_radius;
		_replace_ai_action(entity, action);
	}
}