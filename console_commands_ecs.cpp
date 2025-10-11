#include "stdafx.h"
#include "console_commands.h"
#include "ecs.h"

namespace console {
	void _register_ecs_commands() {
		register_command({
			.name = "debug_terrain",
			.desc = "Debug draw the terrain.",
			.params = { { ParamType::Bool, "on", "Is debug drawing on?" } },
			.callback = [](Args args) { ecs::debug_terrain = get_bool(args[0]); }
			});
	}
}