#include "stdafx.h"
#include "ecs_arrow.h"
#include "ecs_tags.h"
#include "ecs_lifetime.h"
#include "ecs_sprites.h"
#include "ecs_tiled.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_physics_events.h"
#include "ecs_damage.h"
#include "tile_ids.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void _arrow_handle_touch(const TouchEvent& ev) {
		if (ev.type == TouchEventType::ContactBegin) {
			// Destroy the arrow and apply damage to the other entity
			destroy_later(ev.entity);
			deal_damage(ev.other_entity, { .type = DamageType::Projectile, .amount = 1 });
		}
	}

	entt::entity create_arrow(const Vec2f& position, const Vec2f& velocity) {
		entt::entity entity = _registry.create();
		set_tag(entity, Tag::Arrow);
		const Vec2f tile_center = { 8.f, 8.f };
		const Vec2f position_to_top_left = -tile_center - Vec2f(0.f, 16.f);
		if (const TilesetId tileset = get_tileset("items1")) {
			if (const TileId tile = get_tile(tileset, TILE_ID_ITEM_SPEAR)) {
				sprites::Sprite& sprite = emplace_sprite(entity);
				setup_sprite(sprite, tile, true);
				sprite.position = position + position_to_top_left;
				sprite.sorting_layer = get_object_layer();
				sprite.sorting_point = tile_center;
			}
		}
		{
			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_dynamicBody;
			body_def.position = position + position_to_top_left;
			body_def.linearVelocity = velocity;
			b2BodyId body = emplace_body(entity, body_def);
			b2ShapeDef shape_def = b2DefaultShapeDef();
			shape_def.enableContactEvents = true;
			shape_def.filter = get_physics_filter(Tag::Arrow);
			b2Circle circle{};
			circle.center = -position_to_top_left;
			circle.radius = 5.f;
			b2CreateCircleShape(body, &shape_def, &circle);
		}
		set_touch_event_handler(entity, _arrow_handle_touch);
		audio::create_event("event:/snd_fire_arrow", { .position = position });
		return entity;
	}
}