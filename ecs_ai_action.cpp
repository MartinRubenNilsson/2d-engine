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

	std::string to_string(AiActionType type) {
		return std::string(magic_enum::enum_name(type));
	}

	// Imagine a magnetic dipole at the origin, with the north pole pointing in the
	// positive x direction. This function returns a normalized vector that points
	// in the direction of the magnetic field line at the given position. It's fake
	// since the arcs are perfect circles, unlike a real magnetic field, but that's
	// ok since we're just using it to make the pathfinding smoother.
	Vector2f _get_magnetic_field_line_at(const Vector2f& pos) {
		Vector2f result(1.f, 0.f);
		float x2 = pos.x * pos.x;
		float y2 = pos.y * pos.y;
		float r2 = x2 + y2;
		if (r2 > 0.0001f) {
			result.x = (x2 - y2) / r2;
			result.y = 2.f * pos.x * pos.y / r2;
		}
		return result;
	}

	Vector2f _get_magnetic_field_line_at(const Vector2f& pos, float field_rotation) {
		float c = cos(field_rotation);
		float s = sin(field_rotation);
		Vector2f result = _get_magnetic_field_line_at(
			Vector2f(c * pos.x + s * pos.y, -s * pos.x + c * pos.y));
		return Vector2f(c * result.x - s * result.y, s * result.x + c * result.y);
	}

	void update_ai_actions(float dt) {
		_ai_action_time += dt;

		for (auto [entity, action] : _registry.view<AiAction>().each()) {
			if (action.status == AiActionStatus::Running)
				action.running_time += dt;
		}

		for (auto [entity, action, body] : _registry.view<AiAction, b2BodyId>().each()) {
			if (action.status != AiActionStatus::Running) continue;

			//const Vector2f my_pos = b2Body_GetPosition(body);
			const Vector2f my_pos = b2Body_GetWorldCenterOfMass(body);
			const Vector2f my_old_dir = normalize(b2Body_GetLinearVelocity(body));
			Vector2f my_new_dir;

			switch (action.type) {
				case AiActionType::None: {
					// Do nothing.
				} break;
				case AiActionType::Wait: {
					if (action.running_time >= action.duration)
						action.status = AiActionStatus::Succeeded;
				} break;
				case AiActionType::MoveTo: {
					Vector2f to_target = action.position - my_pos;
					float dist = length(to_target);
					if (dist <= action.radius) {
						action.status = AiActionStatus::Succeeded;
					} else {
						my_new_dir = to_target / dist;
					}
				} break;
				case AiActionType::Pursue: {
					b2BodyId target_body = get_body(action.entity);
					if (B2_IS_NULL(target_body)) {
						action.status = AiActionStatus::Failed;
						break;
					}
					Vector2f target_pos = b2Body_GetPosition(target_body);
					Vector2f me_to_target = target_pos - my_pos;
					float dist_to_target = length(me_to_target);
					if (dist_to_target <= action.radius) {
						action.status = AiActionStatus::Succeeded;
						break;
					}
					my_new_dir = me_to_target / dist_to_target;
					if (!action.pathfind)
						break;
					uint32_t my_category_bits = get_category_bits(body);
					uint32_t target_category_bits = get_category_bits(target_body);
					uint32_t mask_bits = ~(my_category_bits | target_category_bits); // Exclude self and target.
					Vector2f strafe_dir = rotate_90deg(my_new_dir);
					// If there's a direct line of sight, don't bother with pathfinding.
					if (!raycast_closest(my_pos + 8.f * strafe_dir, target_pos, mask_bits) &&
						!raycast_closest(my_pos - 8.f * strafe_dir, target_pos, mask_bits)) {
						action.path.clear();
						break;
					}
					Vector2i my_tile = map::world_to_tile(my_pos);
					Vector2i target_tile = map::world_to_tile(target_pos);
					if (my_tile == target_tile)
						break;
					if (!map::pathfind(my_tile, target_tile, action.path)) {
						action.status = AiActionStatus::Failed;
						break;
					}
					Vector2i next_tile = action.path[1];
					Vector2i to_next_tile = next_tile - my_tile;
					Vector2f magnetic_field_origin =
						(map::get_tile_center(my_tile) + map::get_tile_center(next_tile)) * 0.5f;
					Vector2f magnetic_field_to_me = my_pos - magnetic_field_origin;
					float magnetic_field_rotation = atan2((float)to_next_tile.y, (float)to_next_tile.x);
					my_new_dir = _get_magnetic_field_line_at(magnetic_field_to_me, magnetic_field_rotation);
				} break;
				case AiActionType::Flee: {
					b2BodyId danger_body = get_body(action.entity);
					if (B2_IS_NULL(danger_body)) {
						action.status = AiActionStatus::Failed;
						break;
					}
					Vector2f danger_pos = b2Body_GetPosition(danger_body);
					Vector2f to_danger = danger_pos - my_pos;
					float dist = length(to_danger);
					if (dist >= action.radius) {
						action.status = AiActionStatus::Succeeded;
					} else {
						my_new_dir = -(to_danger / dist); // Note the minus sign.
					}
				} break;
				case AiActionType::Wander: {
					if (action.duration > 0.f && action.running_time >= action.duration) {
						action.status = AiActionStatus::Succeeded;
						break;
					}
					if (action.radius <= 0.f) {
						action.status = AiActionStatus::Failed;
						break;
					}
					my_new_dir = my_old_dir;
					if (is_zero(my_new_dir))
						my_new_dir = random::on_circle();
					float noise_sample = random::fractal_perlin_noise(
						action.position.x * 0.01f,
						action.position.y * 0.01f,
						_ai_action_time * 1.f);
					my_new_dir = rotate(my_new_dir, 5.f * noise_sample * dt);
					Vector2f to_center = action.position - my_pos;
					float dist = length(to_center);
					if (dist > action.radius * 0.5f) {
						float lerp_t = std::clamp((dist / action.radius - 0.5f) * 2.f, 0.f, 1.f);
						my_new_dir = lerp_polar(my_new_dir, to_center / dist, lerp_t);
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

	void ai_wait(entt::entity entity, float duration) {
		AiAction action{};
		action.type = AiActionType::Wait;
		action.duration = duration;
		_replace_ai_action(entity, action);
	}

	void ai_move_to(entt::entity entity, const Vector2f& target_position, float speed, float acceptance_radius, bool pathfind) {
		AiAction action{};
		action.type = AiActionType::MoveTo;
		action.position = target_position;
		action.speed = speed;
		action.radius = acceptance_radius;
		action.pathfind = pathfind;
		_replace_ai_action(entity, action);
	}

	void ai_pursue(entt::entity entity, entt::entity target_entity, float speed, float acceptance_radius, bool pathfind) {
		AiAction action{};
		action.type = AiActionType::Pursue;
		action.entity = target_entity;
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

	void ai_wander(entt::entity entity, const Vector2f& wander_center, float speed, float wander_radius, float duration) {
		AiAction action{};
		action.type = AiActionType::Wander;
		action.position = wander_center;
		action.speed = speed;
		action.radius = wander_radius;
		action.duration = duration;
		_replace_ai_action(entity, action);
	}
}