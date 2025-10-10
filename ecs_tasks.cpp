#include "stdafx.h"
#include "ecs_task_impl.h"
#include "ecs_tasks.h"
#include "text.h"
#include "fonts.h"

namespace ecs {
	extern entt::registry _registry;

	std::string_view current_task(entt::entity entity) {
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

	bool _should_do(const Task& task, std::string_view task_name) {
		return !_done(task.status) && task.name == task_name;
	}

	bool done(entt::entity entity) {
		return _done(status(entity));
	}

	void succeed(entt::entity entity) {
		if (Task* task = _registry.try_get<Task>(entity)) {
			task->status = TaskStatus::Succeeded;
		}
	}

	void fail(entt::entity entity) {
		if (Task* task = _registry.try_get<Task>(entity)) {
			task->status = TaskStatus::Failed;
		}
	}

	void new_task(std::string_view name) {
		// TODO
	}

	void then(entt::entity entity, void (*then)(entt::entity entity)) {
		_registry.get_or_emplace<Task>(entity).then_queue.push_back(then);
	}

	void _do_then(entt::entity entity, Task& task) {
		if (task.then_queue.empty())
			return;

		// PITFALL: A "then" callback may want to add more callbacks. This is by design -
		// for example, when the callback starts up a new task. These new callbacks must
		// be added to the FRONT of the queue. This is due to the hierarchical nature of
		// tasks: Chained tasks (and thus their callbacks) happen first; outer tasks and
		// callbacks happen when all the inner tasks are completed.

		// Move queue over to a local variable so it is empty before invoking the callback.
		std::vector<Task::Then> then_queue = std::move(task.then_queue);
		// Invoke the first callback in the queue.
		then_queue.front()(entity);
		// At this point the queue may have grown. Append the remaining callbacks to the end.
		for (size_t i = 1; i < then_queue.size(); ++i) {
			task.then_queue.push_back(then_queue[i]);
		}
	}

	void _update_done_tasks(float dt) {
		for (auto [entity, task] : _registry.view<Task>().each()) {
			if (!_done(task.status))
				continue; // Keep doing task.
			_do_then(entity, task);
		}
	}

	void update_tasks(float dt) {
		_update_wait_tasks(dt);
		_update_wander_tasks(dt);
		_update_pursue_tasks(dt);
		_update_flee_tasks(dt);
		_update_done_tasks(dt); // must be done last!
	}

	void _debug_draw_task_names() {
		text::Text text{};
		text.font = fonts::load_font("assets/fonts/Helvetica.ttf");;
		text.height = 8.f;

		for (auto [entity, body, task] : _registry.view<const b2BodyId, const Task>().each()) {
			if (task.name.empty())
				continue;
			text.string.assign(task.name.begin(), task.name.end());
			text.position = b2Body_GetWorldCenterOfMass(body);
			text.position.y -= 16.f;
			// TODO: text color!!!
			text::draw_later(text);
		}

		text::draw_all_now("ecs::_debug_draw_task_names()");
	}

	void debug_draw_tasks() {
		_debug_draw_pursue_tasks();
		_debug_draw_task_names();
	}
}