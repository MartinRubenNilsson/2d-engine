#include "stdafx.h"
#include "ecs_sprites.h"
#include "ecs_tiled.h"
#include "ecs_uniform_block.h"
#include "random.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "sprites.h"

namespace ecs {
	extern entt::registry _registry;

	sprites::Sprite& emplace_sprite(entt::entity entity) {
		return _registry.emplace_or_replace<sprites::Sprite>(entity);
	}

	void update_sprite(sprites::Sprite& sprite, TileId tile) {
		if (!tile) return;
		const TextureRect rect = get_texture_rect(tile);
		sprite.tex_position = { (float)rect.x, (float)rect.y };
		sprite.tex_size = { (float)rect.w, (float)rect.h };
		sprite.size = sprite.tex_size;
		const TilesetId tileset = get_tileset(tile); // tileset is valid if tile is
		const Vector2u tileset_size = get_size_in_pixels(tileset);
		sprite.tex_position /= Vector2f(tileset_size);
		sprite.tex_size /= Vector2f(tileset_size);
		sprite.texture = graphics::load_texture(get_image_path(tileset));
	}

	sprites::Sprite* get_sprite(entt::entity entity) {
		return _registry.try_get<sprites::Sprite>(entity);
	}

	// Makes the Sprite follow along a b2BodyId as the latter moves.
	struct SpriteFollowBody {
		Vector2f offset; // the sprite's position relative to the body's position
	};

	void make_sprite_follow_body(entt::entity entity, const Vector2f& offset) {
		_registry.emplace_or_replace<SpriteFollowBody>(entity, offset);
	}

	void make_sprite_blink(entt::entity entity, SpriteBlink&& blink) {
		_registry.emplace_or_replace<SpriteBlink>(entity, std::move(blink));
	}

	void make_sprite_shake(entt::entity entity, SpriteShake&& shake) {
		shake._random_seed = random::range_ui(0, 128);
		_registry.emplace_or_replace<SpriteShake>(entity, std::move(shake));
	}

	void setup_sprites() {
		for (auto [entity, tile] : _registry.view<TileId>().each()) {

		}
	}

	void _update_sprites_following_bodies() {
		for (auto [entity, sprite, body, follow] :
			_registry.view<sprites::Sprite, b2BodyId, SpriteFollowBody>().each()) {
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

	void draw_sprites(const Vector2f& camera_min, const Vector2f& camera_max) {
		graphics::ScopedDebugGroup debug_group("ecs::draw_sprites()");

		_blink_sprites_before_drawing();
		_shake_sprites_before_drawing();

		//TODO: don't to frustum culling twice, store ptrs in a vector or something
		//or maybe emplace a tag?

		std::vector<UniformBlock> blocks;

		for (auto [entity, sprite, block] : _registry.view<sprites::Sprite, const UniformBlock>().each()) {
			if (!(sprite.flags & sprites::SPRITE_VISIBLE)) continue;
			if (sprite.position.x > camera_max.x) continue;
			if (sprite.position.y > camera_max.y) continue;
			if (sprite.position.x + sprite.size.x < camera_min.x) continue;
			if (sprite.position.y + sprite.size.y < camera_min.y) continue;
			sprite.uniform_buffer = graphics::sprite_uniform_buffer;
			sprite.uniform_buffer_size = (uint32_t)sizeof(UniformBlock);
			sprite.uniform_buffer_offset = (uint32_t)(blocks.size() * sizeof(UniformBlock));
			blocks.push_back(block);
		}

		graphics::update_buffer(graphics::sprite_uniform_buffer,
			blocks.data(), (unsigned int)blocks.size() * sizeof(UniformBlock));

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

		_unblink_sprites_after_drawing();
		_unshake_sprites_after_drawing();
	}
}