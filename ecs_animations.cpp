#include "stdafx.h"
#include "ecs_animations.h"
#include "ecs_sprites.h"
#include "ecs_tiled.h"
#include "graphics.h"
#include "sprites.h"

namespace ecs {
	extern entt::registry _registry;

	unsigned int TileAnimation::get_frame() const {
		return _frame;
	}

	TileId TileAnimation::get_tile() const {
		return _tile;
	}

	void TileAnimation::set_progress(float progress) {
		_progress = std::clamp(progress, 0.f, 1.f);
	}

	float TileAnimation::get_progress() const {
		return _progress;
	}

	void TileAnimation::set_speed(float speed) {
		_speed = std::max(speed, 0.f);
	}

	float TileAnimation::get_speed() const {
		return _speed;
	}

	void TileAnimation::set_loop(bool loop) {
		_loop = loop;
	}

	bool TileAnimation::get_loop() const {
		return _loop;
	}

	bool TileAnimation::done() const {
		if (_loop) return false;
		return _progress == 1.f;
	}

	bool TileAnimation::looped() const {
		return _looped;
	}

	bool TileAnimation::frame_changed() const {
		return _frame_changed;
	}

	bool TileAnimation::tile_changed() const {
		return _tile_changed;
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
		for (auto [entity, tile, animation] : _registry.view<TileId, TileAnimation>().each()) {

			if (!tile) continue;

			animation._looped = false;
			const unsigned int prev_anim_frame = animation._frame;
			animation._frame = UINT_MAX; // in case the tile isn't animated
			animation._frame_changed = (animation._frame != prev_anim_frame);
			const TileId prev_anim_tile = animation._tile;
			animation._tile = tile; // in case the tile isn't animated
			animation._tile_changed = (animation._tile != prev_anim_tile);

			if (!animated(tile))
				continue;

			const unsigned int duration_ms = get_animation_duration_ms(tile);
			if (duration_ms == 0.f)
				continue; // defensive

			// TODO: support for negative speed
			const float delta_progress = animation._speed * dt / (duration_ms * 0.001f);

			animation._progress += delta_progress;
			if (animation._progress >= 1.f) {
				if (animation._loop) {
					animation._looped = true;
					animation._progress = fmodf(animation._progress, 1.f);
				} else {
					animation._progress = 1.f;
				}
			}

			const unsigned int time_ms = (unsigned int)(animation._progress * duration_ms);
			const TileAnimationFrame frame = get_animation_frame(tile, time_ms);
			animation._frame = frame.index;
			animation._frame_changed = (animation._frame != prev_anim_frame);
			animation._tile = frame.tile;
			animation._tile_changed = (animation._tile != prev_anim_tile);
		}
	}

	void _update_tile_animated_sprites(float dt) {
		for (auto [entity, sprite, animation] : _registry.view<sprites::Sprite, const TileAnimation>().each()) {
			if (!animation._tile_changed)
				continue; // No need to update the sprite if the frame hasn't changed.
			if (!animation._tile)
				continue;
			setup_sprite(sprite, animation._tile, false);
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

	void _update_flipbook_animated_sprites(float dt) {
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

	void setup_animations() {
		_setup_tile_animations();
	}

	void update_animations(float dt) {
		_update_tile_animations(dt);
		_update_flipbook_animations(dt);
	}

	void update_animated_sprites(float dt) {
		_update_tile_animated_sprites(dt);
		_update_flipbook_animated_sprites(dt);
	}
}
