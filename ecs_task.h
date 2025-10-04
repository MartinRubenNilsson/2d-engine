#pragma once

namespace ecs {
	enum class TaskStatus {
		Preparing,
		Doing,
		Succeeded,
		Failed
	};

	struct Task {
		TaskStatus status = TaskStatus::Preparing;
		void (*then)(entt::entity entity) = nullptr;
	};

	TaskStatus status(entt::entity entity);
	bool succeeded(entt::entity entity); // check if status = Succeeded
	bool failed(entt::entity entity); // check if status = Failed

	void then(entt::entity entity, void (*then)(entt::entity entity));

	void wait(entt::entity entity, float time);
	void wander(entt::entity entity, float speed, float radius, float time);

	void update_tasks(float dt);
}