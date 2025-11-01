#include "stdafx.h"
#include "ecs.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_patch.h"
#include "ecs_lifetime.h"
#include "ecs_pathfinding.h"
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
#include "ecs_terrain.h"

namespace ecs {
	void _startup_engine() {
		startup_physics();
		startup_tiled();
	}

	void _startup_game() {
		startup_grass();
	}

	void startup() {
		_startup_engine();
		_startup_game();
	}

	void shutdown() {
		clear();
		shutdown_physics();
	}

	entt::registry _registry;

	void _setup_engine(MapId map) {
		setup_tiled(map);
		setup_tags();
		setup_physics(map);
		setup_pathfinding(map);
		setup_terrain(map);
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
		_setup_engine(map);
		_setup_game(map);
	}

	void _patch_engine(const Patch& patch) {
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
		_patch_engine(patch);
		_patch_game(patch);
	}

	void setup(MapId map) {
		clear();
		if (!map) return;
		_setup(map);
		_patch(map);
	}

	void clear() {
		_registry.clear();
		clear_physics();
		clear_pathfinding();
		clear_terrain();
		clear_entities_to_be_destroyed_later();
	}

	void _update_game_logic(float dt) {
		update_players(dt);
		update_pickups(dt);
		update_bombs(dt);
		update_blade_traps(dt);
	}

	void _update_engine_logic(float dt) {
		update_pathfinding(dt);
		update_state_machines(dt);
		update_tasks(dt);
		update_lifetimes(dt);
		destroy_entities_to_be_destroyed_later();
	}

	void _update_game_graphics(float dt) {
		update_slimes_graphics(dt);
	}

	void _update_engine_graphics(float dt) {
		update_animations(dt);
		update_animated_sprites(dt);
		update_sprites(dt);
		update_cameras(dt);
	}

	void update(float dt) {
		update_physics(dt);
		_update_game_logic(dt);
		_update_engine_logic(dt);
		_update_game_graphics(dt);
		_update_engine_graphics(dt);
		update_audio(dt);
	}

	Rect2f get_camera_view() {
		Rect2f view = ecs::get_blended_camera_view();
		view.min = round(view.min); // snap to pixel
		view.max = round(view.max); // snap to pixel
		return view;
	}
}
