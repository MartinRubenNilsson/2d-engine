#include "stdafx.h"
#include "ecs_task_impl.h"

namespace ecs {
	struct FleeTask {
		entt::entity target_entity = entt::null; // the entity to flee
		float speed = 0.f;
		float safety_radius = 0.f;
	};

	extern entt::registry _registry;

	void flee(entt::entity entity, entt::entity target_entity, float speed, float safety_radius) {
		_registry.emplace_or_replace<Task>(entity, "flee");
		_registry.emplace_or_replace<FleeTask>(entity, target_entity, speed, safety_radius);
	}

	void _update_flee_tasks(float dt) {
		for (auto [entity, task, flee] : _registry.view<Task, FleeTask>().each()) {
			if (!_should(task, "flee")) {
				_registry.erase<FleeTask>(entity);
				continue;
			}
			if (!_registry.all_of<b2BodyId>(entity)) {
				task.status = TaskStatus::Failed;
				_registry.erase<FleeTask>(entity);
				continue;
			}
			if (flee.safety_radius <= 0.f ||
				!_registry.valid(flee.target_entity) ||
				!_registry.all_of<b2BodyId>(flee.target_entity)) {
				task.status = TaskStatus::Succeeded;
				_registry.erase<FleeTask>(entity);
				continue;
			}
			if (task.status == TaskStatus::Preparing) {
				task.status = TaskStatus::Doing;
			}
			const b2BodyId body = _registry.get<b2BodyId>(entity);
			const b2BodyId target_body = _registry.get<b2BodyId>(flee.target_entity);
			const Vec2f pos = b2Body_GetPosition(body);
			const Vec2f target_pos = b2Body_GetPosition(target_body);
			const Vec2f to_target = target_pos - pos;
			const float dist_to_target = length(to_target);
			if (dist_to_target >= flee.safety_radius) {
				task.status = TaskStatus::Succeeded;
				_registry.erase<FleeTask>(entity);
				continue;
			}
			const Vec2f new_dir = -to_target / dist_to_target; // SIC: minus
			b2Body_SetLinearVelocity(body, new_dir * flee.speed);
		}
	}
}