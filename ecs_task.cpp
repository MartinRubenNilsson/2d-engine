#include "stdafx.h"
#include "ecs_task.h"
#include "random.h"

namespace ecs {
	extern entt::registry _registry;

	TaskStatus status(entt::entity entity) {
		const Task* task = _registry.try_get<Task>(entity);
		if (!task) return TaskStatus::Failed;
		return task->status;
	}

	bool succeeded(entt::entity entity) {
		return status(entity) == TaskStatus::Succeeded;
	}

	bool failed(entt::entity entity) {
		return status(entity) == TaskStatus::Failed;
	}

	void then(entt::entity entity, void (*then)(entt::entity entity)) {
		_registry.get_or_emplace<Task>(entity).then = then;
	}

	void _update_wait_tasks(float dt);
	void _update_wander_tasks(float dt);

	void _update_tasks(float dt) {
		for (auto [entity, task] : _registry.view<Task>().each()) {
			if (task.status == TaskStatus::Preparing || task.status == TaskStatus::Doing)
				continue;
			if (task.then) {
				auto then = task.then;
				task.then = nullptr;
				// PITFALL: Calling task.then may lead to it being replaced,
				// which is why we set it to nullptr *before* calling it.
				then(entity);
			}
		}
	}

	void update_tasks(float dt) {
		_update_wait_tasks(dt);
		_update_wander_tasks(dt);
		_update_tasks(dt);
	}
}