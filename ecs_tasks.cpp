#include "stdafx.h"
#include "ecs_task_impl.h"
#include "ecs_tasks.h"
#include "text.h"
#include "fonts.h"

namespace ecs {
	extern entt::registry _registry;

	std::string_view get_current_task(entt::entity entity) {
		const Task* task = _registry.try_get<Task>(entity);
		if (!task) return {};
		return task->name;
	}

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

	bool _done(TaskStatus status) {
		return status == TaskStatus::Succeeded || status == TaskStatus::Failed;
	}

	bool _should(const Task& task, std::string_view task_name) {
		return !_done(task.status) && task.name == task_name;
	}

	bool done(entt::entity entity) {
		return _done(status(entity));
	}

	void then(entt::entity entity, void (*then)(entt::entity entity)) {
		_registry.get_or_emplace<Task>(entity).then = then;
	}

	void _update_tasks_that_are_done(float dt) {
		for (auto [entity, task] : _registry.view<Task>().each()) {
			if (!_done(task.status))
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
		_update_pursue_tasks(dt);
		_update_flee_tasks(dt);
		_update_tasks_that_are_done(dt); // must be done last!
	}

	void _debug_draw_task_names() {
		text::Text text{};
		text.font = fonts::load_font("assets/fonts/Helvetica.ttf");;
		text.pixel_height = 48.f;
		text.scale = { 0.1f, 0.1f };

		for (auto [entity, body, task] : _registry.view<const b2BodyId, const Task>().each()) {
			if (task.name.empty())
				continue;
			text.string.assign(task.name.begin(), task.name.end());
			text.position = b2Body_GetWorldCenterOfMass(body) + Vec2f(-8.f, -10.f);
			// TODO: text color!!!
			text::render(text);
		}
	}

	void debug_draw_tasks() {
		_debug_draw_pursue_tasks();
		_debug_draw_task_names();
	}
}