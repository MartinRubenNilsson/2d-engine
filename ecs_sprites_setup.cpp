#include "stdafx.h"
#include "ecs_sprites.h"
#include "ecs_tiled.h"
#include "graphics.h"

namespace ecs {
	extern entt::registry _registry;

	void setup_sprite(sprites::Sprite& sprite, TileId tile, bool load_texture) {
		if (!tile) return;
		const TextureRect rect = get_texture_rect(tile);
		sprite.tex_position = { (float)rect.x, (float)rect.y };
		sprite.tex_size = { (float)rect.w, (float)rect.h };
		sprite.size = sprite.tex_size;
		const TilesetId tileset = get_tileset(tile); // tileset is valid if tile is
		const Vec2u tileset_size = get_size_in_pixels(tileset);
		sprite.tex_position /= Vec2f(tileset_size);
		sprite.tex_size /= Vec2f(tileset_size);
		if (load_texture) {
			sprite.texture = graphics::load_texture(get_image_path(tileset));
		}
		if (tile.flipped_horizontally) {
			sprite.flags |= sprites::SPRITE_FLIP_HORIZONTALLY;
		} else {
			sprite.flags &= ~sprites::SPRITE_FLIP_HORIZONTALLY;
		}
		if (tile.flipped_vertically) {
			sprite.flags |= sprites::SPRITE_FLIP_VERTICALLY;
		} else {
			sprite.flags &= ~sprites::SPRITE_FLIP_VERTICALLY;
		}
		if (tile.flipped_diagonally) {
			sprite.flags |= sprites::SPRITE_FLIP_DIAGONALLY;
		} else {
			sprite.flags &= ~sprites::SPRITE_FLIP_DIAGONALLY;
		}
	}

	void setup_sprites(MapId map) {

		const Vec2u map_tile_size = get_tile_size(map);

		// Setup sprites for static level geometry. These are the tiles with a coord.
		for (auto [entity, tile, coord] : _registry.view<TileId, TileCoord>().each()) {

			sprites::Sprite& sprite = emplace_sprite(entity, tile);
			sprite.sorting_layer = (uint8_t)coord.layer;

			const Vec2u size = get_size(tile);

			// PITFALL: The tile size may be greater (or smaller) than the tile grid used by the map!
			// In that case the tile's *bottom left* is aligned to the grid cell's bottom left. This
			// means that the tile may extend *above* (or below) the grid cell. We need to compensate
			// for this when we position the sprite.

			const float height_overshoot = (float)size.y - map_tile_size.y;
			sprite.position = {
				(float)coord.x * map_tile_size.x,
				(float)coord.y * map_tile_size.y - height_overshoot // compensate for overshoot
			};

			// Sensible default: Put the sorting point in the center of the tile horizontally
			// but in the center of the *grid cell* vertically. This makes e.g. trees sort correctly.
			sprite.sorting_point = {
				size.x * 0.5f,
				size.y - map_tile_size.y * 0.5f
			};

			// Hide sprites on invisible layers, e.g. the collision tile layer.
			if (!is_layer_visible(map, coord.layer)) {
				sprite.flags &= ~sprites::SPRITE_VISIBLE;
			}
		}

		// Setup sprites for tile objects.
		for (auto [entity, object, tile] : _registry.view<ObjectId, TileId>().each()) {
			sprites::Sprite& sprite = emplace_sprite(entity, tile);

			// PITFALL: get_position() returns the bottom left for tile objects!
			sprite.position = get_top_left(object);

			// PITFALL: We don't set the sorting layer to the layer index here.
			// This is because we want all objects to be on the same layer, so they
			// are rendered in the correct order. This sorting layer may also be the
			// index of a tile layer so that certain static tiles are rendered as if
			// they were objects, e.g. trees and other props.
			sprite.sorting_layer = get_object_layer();

			// sensible default: center the sorting point
			sprite.sorting_point = get_size(object) * 0.5f;
		}

		// For sprites on entities with physics bodies, set the sprite sorting point to the body's AABB center.
		for (auto [entity, sprite, body] : _registry.view<sprites::Sprite, b2BodyId>().each()) {

			// PITFALL: I tried using the local center of mass here first. However, this didn't work
			// for sensors because they don't have mass. Using the AABB seems more robust.
			const Vec2f aabb_center = b2AABB_Center(b2Body_ComputeAABB(body));
			sprite.sorting_point = aabb_center - sprite.position;
		}
	}
}