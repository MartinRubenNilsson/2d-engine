// This header should only be included in implementations of specific tasks.
// Other code should include "ecs_tasks.h" when they want to the task API.

#pragma once
#include "ecs_task_status.h"

namespace ecs {
	struct Task {
		using Then = void (*)(entt::entity entity);

		std::string_view name;
		TaskStatus status = TaskStatus::Preparing;
		std::vector<Then> then_queue;
	};

	bool _done(TaskStatus status);
	bool _should(const Task& task, std::string_view task_name);

	void _update_wait_tasks(float dt);
	void _update_wander_tasks(float dt);
	void _update_pursue_tasks(float dt);
	void _update_flee_tasks(float dt);

	void _debug_draw_pursue_tasks();
}