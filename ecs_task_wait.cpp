#include "stdafx.h"
#include "ecs_task.h"

namespace ecs {
	struct WaitTask {
		float time = 0.f;
	};

	extern entt::registry _registry;

	void wait(entt::entity entity, float time) {
		_registry.emplace_or_replace<Task>(entity);
		_registry.emplace_or_replace<WaitTask>(entity, time);
	}

	void _update_wait_tasks(float dt) {
		for (auto [entity, task, wait] : _registry.view<Task, WaitTask>().each()) {
			if (task.status == TaskStatus::Succeeded || task.status == TaskStatus::Failed) {
				_registry.erase<WaitTask>(entity);
				continue;
			}
			if (task.status == TaskStatus::Preparing) {
				task.status = TaskStatus::Doing;
			}
			wait.time -= dt;
			if (wait.time > 0.f)
				continue;
			task.status = TaskStatus::Succeeded;
			_registry.erase<WaitTask>(entity);
		}
	}
}