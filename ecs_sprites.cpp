#include "stdafx.h"
#include "ecs_sprites.h"
#include "ecs_physics_events.h"
#include "ecs_uniform_block.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "sprites.h"
#include "random.h"

namespace ecs {
	extern entt::registry _registry;

	sprites::Sprite& emplace_sprite(entt::entity entity) {
		return _registry.emplace_or_replace<sprites::Sprite>(entity);
	}

	sprites::Sprite& emplace_sprite(entt::entity entity, TileId tile) {
		sprites::Sprite& sprite = emplace_sprite(entity);
		setup_sprite(sprite, tile, true);
		return sprite;
	}

	void make_sprite_blink(entt::entity entity, SpriteBlink&& blink) {
		_registry.emplace_or_replace<SpriteBlink>(entity, std::move(blink));
	}

	void make_sprite_shake(entt::entity entity, SpriteShake&& shake) {
		shake._random_seed = random::uniform_uint(0, 128);
		_registry.emplace_or_replace<SpriteShake>(entity, std::move(shake));
	}

	void _update_sprites_with_bodies_that_moved() {
		for (const BodyMoveEvent& ev : get_body_move_events()) {
			if (!_registry.valid(ev.entity))
				continue;
			if (!_registry.all_of<sprites::Sprite>(ev.entity))
				continue;
			sprites::Sprite& sprite = _registry.get<sprites::Sprite>(ev.entity);
			sprite.position = ev.position;
		}
	}

	void _update_sprite_blinks(float dt) {
		for (auto [entity, blink] : _registry.view<SpriteBlink>().each()) {
			if (blink.duration > 0.f) {
				blink.duration -= dt;
			}
			if (blink.duration <= 0.f || blink.interval <= 0.f) {
				_registry.erase<SpriteBlink>(entity);
				continue;
			}
		}
	}

	void _update_sprite_shakes(float dt) {
		for (auto [entity, shake] : _registry.view<SpriteShake>().each()) {
			const float last_duration = shake.duration;
			if (shake.duration > 0.f) {
				shake.duration -= dt;
			}
			if (shake.duration <= 0.f) {
				_registry.erase<SpriteShake>(entity);
				continue;
			}
			shake.magnitude *= pow(shake.duration / last_duration, std::max(shake.exponent, 0.f));
		}
	}

	void update_sprites(float dt) {
		_update_sprites_with_bodies_that_moved();
		_update_sprite_blinks(dt);
		_update_sprite_shakes(dt);
	}

	void _blink_sprites_before_drawing() {
		for (auto [entity, sprites, blink] : _registry.view<sprites::Sprite, SpriteBlink>().each()) {
			blink._original_color = sprites.color;
			const float t = fmod(blink.duration, blink.interval);
			sprites.color = (t < blink.interval / 2.f) ? blink.color : blink._original_color;
		}
	}
	void _shake_sprites_before_drawing() {
		for (auto [entity, sprites, shake] : _registry.view<sprites::Sprite, SpriteShake>().each()) {
			shake._original_position = sprites.position;
			sprites.position.x += shake.magnitude *
				random::fractal_perlin_noise(10.f * shake.duration, (float)shake._random_seed, 0.f);
			sprites.position.y += shake.magnitude *
				random::fractal_perlin_noise(10.f * shake.duration, (float)shake._random_seed, 1.f);
		}
	}

	void _unblink_sprites_after_drawing() {
		for (auto [entity, sprites, blink] : _registry.view<sprites::Sprite, SpriteBlink>().each()) {
			sprites.color = blink._original_color;
		}
	}

	void _unshake_sprites_after_drawing() {
		for (auto [entity, sprites, shake] : _registry.view<sprites::Sprite, SpriteShake>().each()) {
			sprites.position = shake._original_position;
		}
	}

	void draw_sprites_now(const Vec2f& camera_min, const Vec2f& camera_max) {

		_blink_sprites_before_drawing();
		_shake_sprites_before_drawing();

		std::vector<UniformBlock> blocks;

		for (auto [entity, sprite] : _registry.view<sprites::Sprite>().each()) {
			if (!(sprite.flags & sprites::SPRITE_VISIBLE)) continue;
			if (sprite.position.x > camera_max.x) continue;
			if (sprite.position.y > camera_max.y) continue;
			if (sprite.position.x + sprite.size.x < camera_min.x) continue;
			if (sprite.position.y + sprite.size.y < camera_min.y) continue;
			sprites::Sprite copy = sprite;
			copy.position = round(copy.position); // snap to pixels
			if (const UniformBlock* block = _registry.try_get<const UniformBlock>(entity)) {
				copy.uniform_buffer = graphics::sprite_uniform_buffer;
				copy.uniform_buffer_size = (uint32_t)sizeof(UniformBlock);
				copy.uniform_buffer_offset = (uint32_t)(blocks.size() * sizeof(UniformBlock));
				blocks.push_back(*block);
			} else {
				copy.uniform_buffer = {};
				copy.uniform_buffer_size = 0;
				copy.uniform_buffer_offset = 0;
			}
			sprites::draw_later(copy);
		}

		if (!blocks.empty()) {
			const graphics::ScopedDebugGroup debug_group(__FUNCTION__);
			graphics::update_buffer(graphics::sprite_uniform_buffer,
				blocks.data(), (unsigned int)blocks.size() * sizeof(UniformBlock));
		}

		sprites::sort_all();
		sprites::draw_all_now(__FUNCTION__);

		_unblink_sprites_after_drawing();
		_unshake_sprites_after_drawing();
	}
}