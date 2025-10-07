#include "stdafx.h"
#include "ecs.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_patch.h"
#include "ecs_lifetime.h"
#include "ecs_physics.h"
#include "ecs_sprites.h"
#include "ecs_player.h"
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
#include "ecs_tasks.h"
#include "ecs_pushable.h"

namespace ecs {
	void startup() {
		startup_physics();
		startup_tiled();
	}

	void shutdown() {
		clear();
		shutdown_physics();
	}

	entt::registry _registry;

	void clear() {
		clear_entities_to_be_destroyed_later();
		_registry.clear();
	}

	void _setup_essentials(MapId map) {
		setup_tiled(map);
		setup_tags();
		setup_physics(map);
		setup_sprites(map);
		setup_animations();
		setup_cameras(map);
		setup_audio();
	}

	void _setup_game(MapId map) {
		setup_portals();
		setup_grass();
		setup_chests();
		setup_players(map);
		setup_slimes();
		setup_blade_traps();
		setup_pushables();
	}

	void _setup(MapId map) {
		_setup_essentials(map);
		_setup_game(map);
	}

	void _patch_essentials(const Patch& patch) {
		patch_entities_to_destroy(patch);
	}

	void _patch_game(const Patch& patch) {
		patch_players(patch);
		patch_chests(patch);
	}

	void _patch(MapId map) {
		map_to_patch = map;
		if (!has_patch()) return;
		Patch& patch = get_patch();
		_patch_essentials(patch);
		_patch_game(patch);
	}

	void setup(MapId map) {
		clear();
		if (!map) return;
		_setup(map);
		_patch(map);
	}

	void _update_game_logic(float dt) {
		update_players(dt);
		update_pickups(dt);
		update_bombs(dt);
		update_blade_traps(dt);
	}

	void _update_essential_logic(float dt) {
		update_state_machines(dt);
		update_tasks(dt);
		update_lifetimes(dt);
		destroy_entities_to_be_destroyed_later();
	}

	void _update_game_graphics(float dt) {
		update_slimes_graphics(dt);
	}

	void _update_essential_graphics(float dt) {
		update_animations(dt);
		update_animated_sprites(dt);
		update_sprites(dt);
		update_cameras(dt);
	}

	void update(float dt) {
		update_physics(dt);
		_update_game_logic(dt);
		_update_essential_logic(dt);
		_update_game_graphics(dt);
		_update_essential_graphics(dt);
		update_audio(dt);
	}

	void handle_window_event(const window::Event& event) {
		handle_window_event_for_players(event);
	}

	void get_camera_bounds(Vec2f& min, Vec2f& max) {
		Vec2f center, size;
		ecs::get_blended_camera_view(center, size);
		min = center - size / 2.f;
		max = center + size / 2.f;
		min = round(min); // snap to pixel
		max = round(max); // snap to pixel
	}

	int debug_flags = 0;

	void add_debug_shapes_to_render_queue() {
		if (debug_flags & DEBUG_PHYSICS) {
			debug_draw_physics();
		}
		if (debug_flags & DEBUG_AI) {
			debug_draw_tasks();
			debug_draw_state_machines();
		}
		if (debug_flags & DEBUG_PLAYER) {
			show_player_debug_window();
		}
	}
}
