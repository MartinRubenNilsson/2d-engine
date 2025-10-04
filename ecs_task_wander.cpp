#include "stdafx.h"
#include "ecs_task.h"
#include "random.h"

namespace ecs {
	struct WanderTask {
		float speed = 0.f;
		float radius = 0.f;
		float time = 0.f;
		Vector2f start_pos;
	};

	extern entt::registry _registry;

	void wander(entt::entity entity, float speed, float radius, float time) {
		_registry.emplace_or_replace<Task>(entity);
		_registry.emplace_or_replace<WanderTask>(entity, speed, radius, time);
	}

	void _update_wander_tasks(float dt) {
		for (auto [entity, task, wander] : _registry.view<Task, WanderTask>().each()) {
			if (task.status == TaskStatus::Succeeded || task.status == TaskStatus::Failed) {
				_registry.erase<WanderTask>(entity);
				continue;
			}
			if (wander.radius <= 0.f || !_registry.all_of<b2BodyId>(entity)) {
				task.status = TaskStatus::Failed;
				_registry.erase<WanderTask>(entity);
				continue;
			}
			const b2BodyId body = _registry.get<b2BodyId>(entity);
			if (task.status == TaskStatus::Preparing) {
				wander.start_pos = b2Body_GetWorldCenterOfMass(body);
				task.status = TaskStatus::Doing;
			}
			wander.time -= dt;
			if (wander.time <= 0.f) {
				b2Body_SetLinearVelocity(body, { 0.f, 0.f }); // stop moving
				task.status = TaskStatus::Succeeded;
				_registry.erase<WanderTask>(entity);
				continue;
			}
			const Vector2f pos = b2Body_GetWorldCenterOfMass(body);
			const Vector2f dir = normalize(b2Body_GetLinearVelocity(body));
			Vector2f new_dir = dir;
			if (is_zero(new_dir))
				new_dir = random::on_circle();
			float noise_sample = random::fractal_perlin_noise(
				wander.start_pos.x * 0.01f,
				wander.start_pos.y * 0.01f,
				wander.time);
			new_dir = rotate(new_dir, 5.f * noise_sample * dt);
			Vector2f to_start = wander.start_pos - pos;
			float dist_to_start = length(to_start);
			if (dist_to_start > wander.radius * 0.5f) {
				float t = std::clamp((dist_to_start / wander.radius - 0.5f) * 2.f, 0.f, 1.f);
				new_dir = lerp_polar(new_dir, to_start / dist_to_start, t);
			}
			b2Body_SetLinearVelocity(body, new_dir * wander.speed);
		}
	}
}