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

	void _handle_physics_for_arrow(const PhysicsEvent& ev) {
		if (ev.type == PhysicsEventType::ContactBeginTouch) {
			// Destroy the arrow and apply damage to the other entity
			destroy_later(ev.entity);
			deal_damage(ev.other_entity, { .type = DamageType::Projectile, .amount = 1 });
		}
	}

	Arrow& emplace_arrow(entt::entity entity, const Arrow& arrow) {
		return _registry.emplace_or_replace<Arrow>(entity, arrow);
	}

	entt::entity create_arrow(const Vec2f& position, const Vec2f& velocity) {
		entt::entity entity = _registry.create();
		set_tag(entity, Tag::Arrow);
		{
			Arrow& arrow = emplace_arrow(entity);
			arrow.damage = 1;
			arrow.lifetime = 0.f; // unused right now
		}
		const Vec2f tile_center = { 8.f, 8.f };
		if (const TilesetId tileset = get_tileset("items1")) {
			if (const TileId tile = get_tile(tileset, TILE_ID_ITEM_SPEAR)) {
				sprites::Sprite& sprite = emplace_sprite(entity);
				setup_sprite(sprite, tile, true);
				sprite.position = position - tile_center;
				sprite.sorting_layer = get_object_layer();
				sprite.sorting_point = tile_center;
			}
		}
		{
			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_dynamicBody;
			body_def.position = position;
			body_def.linearVelocity = velocity;
			b2BodyId body = emplace_body(entity, body_def);
			b2ShapeDef shape_def = b2DefaultShapeDef();
			shape_def.filter = get_physics_filter(Tag::Arrow);
			b2Circle circle{};
			circle.center = tile_center;
			circle.radius = 6.f;
			b2CreateCircleShape(body, &shape_def, &circle);
		}
		set_physics_event_handler(entity, _handle_physics_for_arrow);
		audio::create_event({ .path = "event:/snd_fire_arrow" });
		return entity;
	}
}