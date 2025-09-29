#include "stdafx.h"
#include "ecs.h"
#include "ecs_common.h"
#include "ecs_physics.h"
#include "ecs_sprites.h"
#include "ecs_player.h"
#include "ecs_ai.h"
#include "ecs_animations.h"
#include "ecs_camera.h"
#include "ecs_pickups.h"
#include "ecs_bomb.h"
#include "ecs_portal.h"
#include "ecs_blade_trap.h"
#include "ecs_states.h"
#include "ecs_chest.h"
#include "ecs_grass.h"
#include "ecs_audio.h"
#include "ecs_slime.h"

namespace ecs {
	void initialize() {
		initialize_physics();
	}

	void shutdown() {
		clear();
		shutdown_physics();
	}

	void setup() {
		setup_cameras();
		setup_physics();
		setup_audio_sources();
		setup_portals();
		setup_grass();
		setup_chests();
		setup_players();
		setup_slimes();
		setup_blade_traps();
	}

	void update(float dt) {
		update_physics(dt);
		update_players(dt);
		update_portals(dt);
		update_pickups(dt);
		update_bombs(dt);
		update_blade_traps(dt);
		update_state_machines(dt);
		update_ai_logic(dt); // TODO remove
		update_ai_graphics(dt); // TODO remove
		update_lifetimes(dt);
		destroy_entities_to_be_destroyed_at_end_of_frame();
		update_tile_animations(dt);
		update_flipbook_animations(dt);
		update_animated_sprites(dt);
		update_sprites(dt);
		update_cameras(dt);
	}

	void handle_window_event(const window::Event& event) {
		handle_window_event_for_players(event);
	}

	void get_camera_bounds(Vector2f& min, Vector2f& max) {
		Vector2f center, size;
		ecs::get_blended_camera_view(center, size);
		min = center - size / 2.f;
		max = center + size / 2.f;
	}

	int debug_flags = 0;

	void add_debug_shapes_to_render_queue() {
		if (debug_flags & DEBUG_PHYSICS) {
			debug_draw_physics();
		}
		if (debug_flags & DEBUG_AI) {
			debug_draw_state_machines();
			debug_draw_ai();
		}
		if (debug_flags & DEBUG_PLAYER) {
			show_player_debug_window();
		}
	}

	entt::registry _registry;
}
