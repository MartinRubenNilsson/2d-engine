#include "stdafx.h"
#include "ecs_animations.h"
#include "ecs_tiled.h"
#include "graphics.h"
#include "sprites.h"

namespace ecs {
	extern entt::registry _registry;

	void setup_tile_animations() {
		for (auto [entity, object] : _registry.view<ObjectId>().each()) {
			// Only add a tile animation if the object is a tile object.
			if (get_type(object) != ObjectType::Tile)
				continue;
			const TileId tile = get_tile(object);
			if (!tile)
				continue;
			TileAnimation& animation = emplace_tile_animation(entity);
			animation.tile = tile;
			animation._frame = tile; // DEFENSIVE
		}

		for (auto [entity, tile] : _registry.view<TileId>().each()) {
			// The majority of tiles are not animated and don't change during gameplay,
			// so let's only add an animation component if the tile is actually animated.
			if (!animated(tile))
				continue;
			TileAnimation& animation = emplace_tile_animation(entity);
			animation.tile = tile;
			animation._frame = tile; // DEFENSIVE
		}
	}

	void update_tile_animations(float dt) {
		for (auto [entity, animation] : _registry.view<TileAnimation>().each()) {
			animation._frame_changed = false;
			animation._looped = false;

			if (!animation.tile)
				continue;
			if (!animated(animation.tile))
				continue;

			const unsigned int duration_ms = get_animation_duration(animation.tile);
			if (duration_ms == 0)
				continue; // DEFENSIVE

			// TODO: support for negative speed
			const float delta_progress = animation.speed * dt * 1000.f / duration_ms;

			animation.progress += delta_progress;
			if (animation.progress >= 1.f) {
				if (animation.loop) {
					animation._looped = true;
					animation.progress = fmodf(animation.progress, 1.f);
				} else {
					animation.progress = 1.f;
				}
			}

			unsigned int time_ms = (unsigned int)(animation.progress * duration_ms);
			const TileId new_frame = get_animation_frame(animation.tile, time_ms);
			if (animation._frame != new_frame) {
				animation._frame = new_frame;
				animation._frame_changed = true;
			}
		}
	}

	void update_flipbook_animations(float dt) {
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

	void update_animated_sprites(float dt) {
		for (auto [entity, sprite, animation] : _registry.view<sprites::Sprite, const TileAnimation>().each()) {

			if (!animation._frame_changed)
				continue; // No need to update the sprite if the frame hasn't changed.
			if (!animation._frame)
				continue;

			Vector2u texture_size;
			graphics::get_texture_size(sprite.texture, texture_size.x, texture_size.y);
			if (!texture_size.x || !texture_size.y) continue;

			const TextureRect tex_rect = get_texture_rect(animation._frame);
			sprite.tex_position = { (float)tex_rect.x, (float)tex_rect.y };
			sprite.tex_size = { (float)tex_rect.w, (float)tex_rect.h };
			sprite.tex_position /= Vector2f(texture_size);
			sprite.tex_size /= Vector2f(texture_size);
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

	TileAnimation& emplace_tile_animation(entt::entity entity) {
		return _registry.emplace_or_replace<TileAnimation>(entity);
	}

	TileAnimation* get_tile_animation(entt::entity entity) {
		return _registry.try_get<TileAnimation>(entity);
	}

	FlipbookAnimation& emplace_flipbook_animation(entt::entity entity) {
		return _registry.emplace_or_replace<FlipbookAnimation>(entity);
	}
}
