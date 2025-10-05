#include "stdafx.h"
#include "ecs_animations.h"
#include "ecs_tiled.h"
#include "graphics.h"
#include "sprites.h"

namespace ecs {
	extern entt::registry _registry;

	void TileAnimation::set_progress(float progress) {
		_progress = std::clamp(progress, 0.f, 1.f);
	}

	void TileAnimation::set_speed(float speed) {
		_speed = std::max(speed, 0.f);
	}

	void TileAnimation::set_loop(bool loop) {
		_loop = loop;
	}

	bool TileAnimation::done() const {
		if (_loop) return false;
		return _progress == 1.f;
	}

	bool TileAnimation::looped() const {
		return _looped;
	}

	TileAnimation& emplace_tile_animation(entt::entity entity) {
		return _registry.emplace_or_replace<TileAnimation>(entity);
	}

	void _setup_tile_animations() {
		for (auto [entity, tile] : _registry.view<TileId>().each()) {
			// The majority of tiles are not animated and don't change during gameplay,
			// so let's only add an animation component if the tile is actually animated.
			if (!animated(tile))
				continue;
			emplace_tile_animation(entity);
		}
	}

	void _update_tile_animations(float dt) {
		for (auto [entity, tile, animation] : _registry.view<const TileId, TileAnimation>().each()) {

			if (!tile) continue;

			const TileId old_frame = animation._frame;
			animation._frame = tile; // in case the tile isn't animated
			animation._frame_changed = (animation._frame != old_frame);
			animation._looped = false;

			if (!animated(tile))
				continue;

			const unsigned int duration_ms = get_animation_duration(tile);
			if (duration_ms == 0)
				continue; // defensive

			// TODO: support for negative speed
			const float delta_progress = animation._speed * dt * 1000.f / duration_ms;

			animation._progress += delta_progress;
			if (animation._progress >= 1.f) {
				if (animation._loop) {
					animation._looped = true;
					animation._progress = fmodf(animation._progress, 1.f);
				} else {
					animation._progress = 1.f;
				}
			}

			unsigned int time_ms = (unsigned int)(animation._progress * duration_ms);
			animation._frame = get_animation_frame(tile, time_ms);
			animation._frame_changed = (animation._frame != old_frame);
		}
	}

	FlipbookAnimation& emplace_flipbook_animation(entt::entity entity) {
		return _registry.emplace_or_replace<FlipbookAnimation>(entity);
	}

	void _update_flipbook_animations(float dt) {
		for (auto [entity, animation] : _registry.view<FlipbookAnimation>().each()) {
			if (animation.rows <= 0) continue;
			if (animation.columns <= 0) continue;
			if (animation.fps <= 0.f) continue;
			const float duration = animation.rows * animation.columns / animation.fps;
			animation.time += dt;
			if (animation.time >= duration) {
				animation.time = fmodf(animation.time, duration);
			}
		}
	}

	void setup_animations() {
		_setup_tile_animations();
	}

	void update_animations(float dt) {
		_update_tile_animations(dt);
		_update_flipbook_animations(dt);
	}

	void update_animated_sprites(float dt) {
		for (auto [entity, sprite, animation] : _registry.view<sprites::Sprite, const TileAnimation>().each()) {

			if (!animation._frame_changed)
				continue; // No need to update the sprite if the frame hasn't changed.
			if (!animation._frame)
				continue;

			Vec2u texture_size;
			graphics::get_texture_size(sprite.texture, texture_size.x, texture_size.y);
			if (!texture_size.x || !texture_size.y) continue;

			const TextureRect tex_rect = get_texture_rect(animation._frame);
			sprite.tex_position = { (float)tex_rect.x, (float)tex_rect.y };
			sprite.tex_size = { (float)tex_rect.w, (float)tex_rect.h };
			sprite.tex_position /= Vec2f(texture_size);
			sprite.tex_size /= Vec2f(texture_size);
		}

		for (auto [entity, sprite, animation] : _registry.view<sprites::Sprite, const FlipbookAnimation>().each()) {
			if (animation.rows <= 0) continue;
			if (animation.columns <= 0) continue;
			const unsigned int frame = (unsigned int)(animation.time * animation.fps);
			const unsigned int row = frame / animation.columns;
			const unsigned int col = frame % animation.columns;
			const float frame_width = 1.f / animation.columns;
			const float frame_height = 1.f / animation.rows;
			sprite.tex_position = { col * frame_width, row * frame_height };
			sprite.tex_size = { frame_width, frame_height };
		}
	}
}
