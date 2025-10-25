#include "stdafx.h"
#include "console_commands.h"
#include "ecs.h"
#include "ecs_debugging.h"

namespace console {
	void _create_ecs_commands() {
		create_command({
			.name = "toggle_debug_physics",
			.desc = "Toggle debug drawing of the physics world.",
			.execute = [](Args) { ecs::debug_physics = !ecs::debug_physics; }
			});
		create_command({
			.name = "toggle_debug_terrain",
			.desc = "Toggle debug drawing of the terrain.",
			.execute = [](Args args) { ecs::debug_terrain = !ecs::debug_terrain; }
			});
		create_command({
			.name = "toggle_debug_states",
			.desc = "Toggle debug drawing of state machines.",
			.execute = [](Args args) { ecs::debug_states = !ecs::debug_states; }
			});
		create_command({
			.name = "toggle_debug_tasks",
			.desc = "Toggle debug drawing of tasks.",
			.execute = [](Args args) { ecs::debug_tasks = !ecs::debug_tasks; }
			});
	}
}