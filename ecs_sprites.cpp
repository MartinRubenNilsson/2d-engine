#include "stdafx.h"
#include "ecs_sprites.h"
#include "ecs_tiled.h"
#include "ecs_uniform_block.h"
#include "sprites.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "random.h"
#include "console.h"

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

	void setup_sprites(MapId map) {
		for (auto [entity, object] : _registry.view<ObjectId>().each()) {
			if (!object) {
				console::log_error("Error 56702: Invalid object in setup_sprites()");
				continue;
			}
			if (get_type(object) != ObjectType::Tile)
				continue;

			const TileId tile = get_tile(object);
			if (!tile) {
				console::log_error("Error 18639: Invalid tile for object " + std::string(get_name(object)));
				continue;
			}

			sprites::Sprite& sprite = emplace_sprite(entity);
			update_sprite(sprite, tile);
			// PITFALL: We don't set the sorting layer to the layer index here.
			// This is because we want all objects to be on the same layer, so they
			// are rendered in the correct order. This sorting layer may also be the
			// index of a tile layer so that certain static tiles are rendered as if
			// they were objects, e.g. trees and other props.
			sprite.sorting_layer = get_object_layer();
			sprite.sorting_point = get_size(object) * 0.5f; // sensible default: center the sorting point
			sprite.position = get_top_left(object); // PITFALL: get_position() returns the bottom left for tile objects!

			// TODO: fix flip flags!!!
#if 0
			if (!layer.visible) {
				sprite.flags &= ~sprites::SPRITE_VISIBLE;
			}
			if (object.tile.flipped_horizontally) {
				sprite.flags |= sprites::SPRITE_FLIP_HORIZONTALLY;
			}
			if (object.tile.flipped_vertically) {
				sprite.flags |= sprites::SPRITE_FLIP_VERTICALLY;
			}
			if (object.tile.flipped_diagonally) {
				sprite.flags |= sprites::SPRITE_FLIP_DIAGONALLY;
			}
#endif
		}

		const Vector2u map_tile_size = get_tile_size(map);

		for (auto [entity, tile, coord] : _registry.view<TileId, TileCoord>().each()) {
			if (!tile) {
				console::log_error("Error 91324: Invalid tile in setup_sprites()");
				continue;
			}
			const Vector2u size = get_size(tile); // may be different from map_tile_size!

			sprites::Sprite& sprite = emplace_sprite(entity);
			update_sprite(sprite, tile);
			sprite.sorting_layer = coord.layer;
			// Sensible default: Put the sorting point in the center, or when the tile is
			// longer than the map grid cell height, put it in the center horizontally and
			// half a grid cell away from the bottom edge. This makes e.g. trees sort correctly.
			sprite.sorting_point = {
				size.x * 0.5f,
				size.y - map_tile_size.y * 0.5f
			};
			// We need to compensate for the fact that the tileset tile size may be
			// different from the map tileset size.
			sprite.position = {
				(float)coord.x * map_tile_size.x,
				(float)coord.y * map_tile_size.y - size.y + map_tile_size.y
			};

			// TODO: fix flip flags!!!
#if 0
			if (!layer.visible) {
				sprite.flags &= ~sprites::SPRITE_VISIBLE;
			}
			if (gid.flipped_horizontally) {
				sprite.flags |= sprites::SPRITE_FLIP_HORIZONTALLY;
			}
			if (gid.flipped_vertically) {
				sprite.flags |= sprites::SPRITE_FLIP_VERTICALLY;
			}
			if (gid.flipped_diagonally) {
				sprite.flags |= sprites::SPRITE_FLIP_DIAGONALLY;
			}
#endif
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