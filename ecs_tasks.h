#pragma once
#include "ecs_task_status.h"

namespace ecs {
	std::string_view get_current_task(entt::entity entity);
	TaskStatus status(entt::entity entity);
	bool succeeded(entt::entity entity); // check if status = Succeeded
	bool failed(entt::entity entity); // check if status = Failed
	bool done(entt::entity entity); // check if status = Succeeded or Failed

	// Set a callback to be called after the task is done.
	void then(entt::entity entity, void (*then)(entt::entity entity));

	void wait(entt::entity entity, float time);
	void wander(entt::entity entity, float speed, float radius, float time);
	void pursue(entt::entity entity, entt::entity target, float speed, float acceptance_radius, bool pathfind = true);
	void flee(entt::entity entity, entt::entity target_entity, float speed, float safety_radius);

	void update_tasks(float dt);
	void debug_draw_tasks();
}