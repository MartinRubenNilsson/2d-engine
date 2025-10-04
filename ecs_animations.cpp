#include "stdafx.h"
#include "ecs_animations.h"
#include "ecs_tiled.h"
#include "tiled.h"
#include "graphics.h"
#include "sprites.h"

namespace ecs {
	extern entt::registry _registry;
	extern tiled::Context _tiled_context;

#if 0
	void TileAnimation::set_rotation(int clockwise_quarter_turns) {
		bool bit_0 = _get_nth_bit(clockwise_quarter_turns, 0);
		bool bit_1 = _get_nth_bit(clockwise_quarter_turns, 1);
		set_flag(TILE_FLIP_X, bit_0 != bit_1); // XOR
		set_flag(TILE_FLIP_Y, bit_1);
		set_flag(TILE_FLIP_DIAGONAL, bit_0);
	}
#endif

	unsigned int get_tileset_id(std::string_view name) {
		return get_tileset(name).id;
	}

	Handle<graphics::Texture> get_tileset_texture(unsigned int tileset_id) {
		TilesetId tileset{ (uint16_t)tileset_id };
		if (!valid(tileset)) return Handle<graphics::Texture>();
		return graphics::load_texture(get_image_path(tileset));
	}

	void setup_tile_animations() {

	}

	void update_tile_animations(float dt) {
		for (auto [entity, animation] : _registry.view<TileAnimation>().each()) {

			animation._dirty = false;
			animation._looped = false;

			if (animation.tile_id != animation._previous_tile_id) {
				animation._previous_tile_id = animation.tile_id;
				animation._animated_tile_id = animation.tile_id;
				animation._frame_id = 0;
				// HACK: keep progress when changing tile_id so player walk/run animations don't restart
				//animation.progress = 0.f;
				animation._dirty = true;
			}

			if (animation.tileset_id >= _tiled_context.tilesets.size())
				continue;
			const tiled::Tileset& tileset = _tiled_context.tilesets[animation.tileset_id];
			if (animation.tile_id >= tileset.tiles.size())
				continue;
			const tiled::Tile& tile = tileset.tiles[animation.tile_id];

			if (tile.animation.empty()) continue;

			unsigned int duration_ms = 0; // in milliseconds
			for (const tiled::Frame& frame : tile.animation) {
				duration_ms += frame.duration_ms;
			}

			if (!duration_ms) continue;

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

			for (unsigned int frame_id = 0; frame_id < tile.animation.size(); ++frame_id) {
				const tiled::Frame& frame = tile.animation[frame_id];
				if (time_ms >= frame.duration_ms) {
					time_ms -= frame.duration_ms;
					continue;
				}
				if (frame_id != animation._frame_id) {
					animation._animated_tile_id = frame.tile_id;
					animation._frame_id = frame_id;
					animation._dirty = true;
				}
				break;
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

			if (!animation._dirty) continue;

			TileId tile{
				.id = (uint16_t)animation._animated_tile_id,
				.tileset_id = (uint16_t)animation.tileset_id
			};
			if (!valid(tile))
				continue;

			Vector2u texture_size;
			graphics::get_texture_size(sprite.texture, texture_size.x, texture_size.y);
			if (!texture_size.x || !texture_size.y) continue;

			const TextureRect tex_rect = get_texture_rect(tile);
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
