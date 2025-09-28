#include "stdafx.h"

#include "ecs.h"
#include "ecs_common.h"
#include "ecs_physics.h"
#include "ecs_uniform_block.h"
#include "ecs_sprites.h"
#include "ecs_player.h"
#include "ecs_ai.h"
#include "ecs_animations.h"
#include "ecs_camera.h"
#include "ecs_vfx.h"
#include "ecs_pickups.h"
#include "ecs_bomb.h"
#include "ecs_portal.h"
#include "ecs_blade_trap.h"
#include "ecs_state_machine.h"

#include "console.h"
#include "sprites.h"
#include "graphics.h"
#include "graphics_globals.h"

namespace ecs {
	int debug_flags = 0;
	entt::registry _registry;

	void initialize() {
		initialize_physics();
	}

	void shutdown() {
		clear();
		shutdown_physics();
	}

	void process_window_event(const window::Event& event) {
		process_window_event_for_players(event);
	}

	void initialize_new_entities() {
		initialize_blade_traps();
	}

	void update(float dt) {
		update_physics(dt);
		update_players(dt);
		update_portals(dt);
		update_pickups(dt);
		update_bombs(dt);
		update_blade_traps(dt);
		update_state_machines(dt);
		update_ai_logic(dt);
		update_ai_graphics(dt);
		update_lifetimes(dt);
		destroy_entities_to_be_destroyed_at_end_of_frame();
		update_tile_animations(dt);
		update_flipbook_animations(dt);
		update_animated_sprites(dt);
		update_sprites_following_bodies();
		update_sprite_blinks(dt);
		update_sprite_shakes(dt);
		update_cameras(dt);
	}

	void get_camera_bounds(Vector2f& min, Vector2f& max) {
		Vector2f center, size;
		ecs::get_blended_camera_view(center, size);
		min = center - size / 2.f;
		max = center + size / 2.f;
	}

	//TODO: move this to a separate file
	std::vector<UniformBlock> _uniform_blocks;

	void draw_sprites(const Vector2f& camera_min, const Vector2f& camera_max) {
		graphics::ScopedDebugGroup debug_group("ecs::draw_sprites()");

		blink_sprites_before_drawing();
		shake_sprites_before_drawing();

		//TODO: don't to frustum culling twice, store ptrs in a vector or something
		//or maybe emplace a tag?

		_uniform_blocks.clear();

		for (auto [entity, sprite, block] : _registry.view<sprites::Sprite, const UniformBlock>().each()) {
			if (!(sprite.flags & sprites::SPRITE_VISIBLE)) continue;
			if (sprite.position.x > camera_max.x) continue;
			if (sprite.position.y > camera_max.y) continue;
			if (sprite.position.x + sprite.size.x < camera_min.x) continue;
			if (sprite.position.y + sprite.size.y < camera_min.y) continue;
			sprite.uniform_buffer = graphics::sprite_uniform_buffer;
			sprite.uniform_buffer_size = (uint32_t)sizeof(UniformBlock);
			sprite.uniform_buffer_offset = (uint32_t)(_uniform_blocks.size() * sizeof(UniformBlock));
			_uniform_blocks.push_back(block);
		}

		graphics::update_buffer(graphics::sprite_uniform_buffer,
			_uniform_blocks.data(), (unsigned int)_uniform_blocks.size() * sizeof(UniformBlock));

		for (auto [entity, sprite] : _registry.view<const sprites::Sprite>().each()) {
			if (!(sprite.flags & sprites::SPRITE_VISIBLE)) continue;
			if (sprite.position.x > camera_max.x) continue;
			if (sprite.position.y > camera_max.y) continue;
			if (sprite.position.x + sprite.size.x < camera_min.x) continue;
			if (sprite.position.y + sprite.size.y < camera_min.y) continue;
			sprites::add(sprite);
		}

		sprites::sort();
		sprites::draw();

		unblink_sprites_after_drawing();
		unshake_sprites_after_drawing();
	}

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
