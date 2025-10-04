#include "stdafx.h"
#include "ecs_arrow.h"
#include "ecs_lifetime.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_damage.h"
#include "tile_ids.h"
#include "audio.h"

namespace map {
	unsigned int get_object_layer_index();
}

namespace ecs {
	extern entt::registry _registry;

	void _on_arrow_physics_event(const PhysicsEvent& ev) {
		if (ev.type == PhysicsEventType::ContactBeginTouch) {
			// Destroy the arrow and apply damage to the other entity
			destroy_at_end_of_frame(ev.entity_a);
			apply_damage(ev.entity_b, { .type = DamageType::Projectile, .amount = 1 });
		}
	}

	Arrow& emplace_arrow(entt::entity entity, const Arrow& arrow) {
		return _registry.emplace_or_replace<Arrow>(entity, arrow);
	}

	entt::entity create_arrow(const Vector2f& position, const Vector2f& velocity) {
		entt::entity entity = _registry.create();
		set_tag(entity, Tag::Arrow);
		{
			Arrow& arrow = emplace_arrow(entity);
			arrow.damage = 1;
			arrow.lifetime = 0.f; // unused right now
		}
		const Vector2f pivot = { 8.f, 8.f };
		if (const TilesetId tileset = get_tileset("items1")) {
			if (const TileId tile = get_tile(tileset, TILE_ID_ITEM_SPEAR)) {
				TileAnimation& animation = emplace_tile_animation(entity);
				animation.tile = tile;

				sprites::Sprite& sprite = emplace_sprite(entity);
				sprite.texture = get_texture(tileset);
				sprite.sorting_layer = map::get_object_layer_index();
				sprite.sorting_point = pivot;
				sprite.position = position - pivot;
				sprite.size = { 16.f, 16.f };
			}
		}
		{
			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_dynamicBody;
			body_def.position = position;
			body_def.linearVelocity = velocity;
			b2BodyId body = emplace_body(entity, body_def);
			b2ShapeDef shape_def = b2DefaultShapeDef();
			shape_def.filter = get_physics_filter_for_tag(Tag::Arrow);
			b2Circle circle{};
			circle.radius = 6.f;
			b2CreateCircleShape(body, &shape_def, &circle);
		}
		set_physics_event_handler(entity, _on_arrow_physics_event);
		make_sprite_follow_body(entity, -pivot);
		audio::create_event({ .path = "event:/snd_fire_arrow" });
		return entity;
	}
}