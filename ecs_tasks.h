#pragma once
#include "ecs_task_status.h"

namespace ecs {
	std::string_view current_task(entt::entity entity);

	TaskStatus status(entt::entity entity);
	bool succeeded(entt::entity entity); // check if status = Succeeded
	bool failed(entt::entity entity); // check if status = Failed
	bool done(entt::entity entity); // check if status = Succeeded or Failed

	void succeed(entt::entity entity); // Make the current task succeed immediately.
	void fail(entt::entity entity); // Make the current task succeed immediately.

	// Indicate the start of a new named task. Calling this is optional.
	void new_task(std::string_view name);
	// Set a callback to be called after the task is done.
	void then(entt::entity entity, void (*then)(entt::entity entity));

	void wait(entt::entity entity, float time);
	void wander(entt::entity entity, float speed, float radius, float time);
	void pursue(entt::entity entity, entt::entity target, float speed, float acceptance_radius, bool pathfind = true);
	void flee(entt::entity entity, entt::entity target, float speed, float safety_radius);

	void update_tasks(float dt);
	void debug_draw_tasks();
}