#include "stdafx.h"
#include "ecs_debugging.h"
#include "ecs_physics.h"
#include "ecs_terrain.h"
#include "ecs_states.h"
#include "ecs_tasks.h"

namespace ecs {
	bool debug_physics = false;
	bool debug_terrain = false;
	bool debug_states = false;
	bool debug_tasks = false;

	void debug() {
		if (debug_physics) {
			debug_draw_physics();
		}
		if (debug_terrain) {
			debug_draw_terrain();
		}
		if (debug_states) {
			debug_draw_state_machines();
		}
		if (debug_tasks) {
			debug_draw_tasks();
		}
	}
}