#include "stdafx.h"
#include "ecs.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_patch.h"
#include "ecs_lifetime.h"
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
	void startup() {
		startup_physics();
		startup_tiled();
	}

	void shutdown() {
		clear();
		shutdown_physics();
	}

	entt::registry _registry;

	entt::entity deep_copy(entt::entity entity) {
		entt::entity copied_entity = _registry.create();
		for (auto [name, storage] : _registry.storage()) {
			if (!storage.contains(entity)) continue;
			if (storage.type() == entt::type_id<b2BodyId>()) {
				deep_copy_and_emplace_body(copied_entity, *(b2BodyId*)storage.value(entity));
			} else if (storage.type() == entt::type_id<Player>()) {
				// TODO: deep copy player, since it holds a held item entity
				storage.push(copied_entity, storage.value(entity));
			} else {
				storage.push(copied_entity, storage.value(entity));
			}
		}
		return copied_entity;
	}

	void clear() {
		clear_entities_to_destroy_at_end_of_frame();
		_registry.clear();
	}

	void _setup(MapId map) {
		setup_tiled(map);
		setup_tags();
		setup_sprites();
		setup_tile_animations();
		setup_physics();
		setup_cameras(map);
		setup_audio_sources();
		setup_portals();
		setup_grass();
		setup_chests();
		setup_players(map);
		setup_slimes();
		setup_blade_traps();
	}

	void _patch(const std::string& patch_id) {
		if (!set_patch(patch_id)) return;
		Patch& patch = get_patch();
		patch_entities_to_destroy(patch);
		patch_chests(patch);
	}

	void setup(const std::string& map_path) {
		clear();
		const MapId map = get_map(map_path);
		if (!map) return;
		_setup(map);
		_patch(map_path);
	}

	void update(float dt) {
		update_physics(dt);
		update_players(dt);
		update_portals(dt);
		update_pickups(dt);
		update_bombs(dt);
		update_blade_traps(dt);
		update_slimes(dt);
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
}
