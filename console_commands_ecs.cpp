#include "stdafx.h"
#include "console_commands.h"
#include "ecs.h"

namespace console {
	void _register_ecs_commands() {
		register_command({
			.name = "toggle_debug_physics",
			.desc = "Toggle debug drawing of the physics world.",
			.callback = [](Args) { ecs::debug_physics = !ecs::debug_physics; }
			});
		register_command({
			.name = "toggle_debug_terrain",
			.desc = "Toggle debug drawing of the terrain.",
			.callback = [](Args args) { ecs::debug_terrain = !ecs::debug_terrain; }
			});
		register_command({
			.name = "toggle_debug_states",
			.desc = "Toggle debug drawing of state machines.",
			.callback = [](Args args) { ecs::debug_states = !ecs::debug_states; }
			});
		register_command({
			.name = "toggle_debug_tasks",
			.desc = "Toggle debug drawing of tasks.",
			.callback = [](Args args) { ecs::debug_tasks = !ecs::debug_tasks; }
			});
	}
}