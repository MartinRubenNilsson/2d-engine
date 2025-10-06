#include "stdafx.h"
#include "ecs_sprites.h"
#include "ecs_uniform_block.h"
#include "sprites.h"
#include "graphics.h"
#include "graphics_globals.h"
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

	sprites::Sprite* get_sprite(entt::entity entity) {
		return _registry.try_get<sprites::Sprite>(entity);
	}

	// Makes the Sprite follow along a b2BodyId as the latter moves.
	struct SpriteFollowBody {
		Vec2f offset; // the sprite's position relative to the body's position
	};

	void make_sprite_follow_body(entt::entity entity, const Vec2f& offset) {
		_registry.emplace_or_replace<SpriteFollowBody>(entity, offset);
	}

	void make_sprite_blink(entt::entity entity, SpriteBlink&& blink) {
		_registry.emplace_or_replace<SpriteBlink>(entity, std::move(blink));
	}

	void make_sprite_shake(entt::entity entity, SpriteShake&& shake) {
		shake._random_seed = random::range_ui(0, 128);
		_registry.emplace_or_replace<SpriteShake>(entity, std::move(shake));
	}

	void _update_sprites_following_bodies() {
		for (auto [entity, sprite, body, follow] : _registry.view<sprites::Sprite, b2BodyId, SpriteFollowBody>().each()) {
			sprite.position = b2Body_GetPosition(body) + follow.offset;
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
		_update_sprites_following_bodies();
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

	void draw_sprites(const Vec2f& camera_min, const Vec2f& camera_max) {
		graphics::ScopedDebugGroup debug_group("ecs::draw_sprites()");

		_blink_sprites_before_drawing();
		_shake_sprites_before_drawing();

		std::vector<UniformBlock> blocks;

		for (auto [entity, sprite] : _registry.view<sprites::Sprite>().each()) {
			if (!(sprite.flags & sprites::SPRITE_VISIBLE)) continue;
			if (sprite.position.x > camera_max.x) continue;
			if (sprite.position.y > camera_max.y) continue;
			if (sprite.position.x + sprite.size.x < camera_min.x) continue;
			if (sprite.position.y + sprite.size.y < camera_min.y) continue;
			if (const UniformBlock* block = _registry.try_get<const UniformBlock>(entity)) {
				sprite.uniform_buffer = graphics::sprite_uniform_buffer;
				sprite.uniform_buffer_size = (uint32_t)sizeof(UniformBlock);
				sprite.uniform_buffer_offset = (uint32_t)(blocks.size() * sizeof(UniformBlock));
				blocks.push_back(*block);
			} else {
				sprite.uniform_buffer = {};
				sprite.uniform_buffer_size = 0;
				sprite.uniform_buffer_offset = 0;
			}
			sprites::add(sprite);
		}

		graphics::update_buffer(graphics::sprite_uniform_buffer,
			blocks.data(), (unsigned int)blocks.size() * sizeof(UniformBlock));

		sprites::sort();
		sprites::draw();

		_unblink_sprites_after_drawing();
		_unshake_sprites_after_drawing();
	}
}