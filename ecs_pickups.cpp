#include "stdafx.h"
#include "ecs_pickups.h"
#include "ecs_lifetime.h"
#include "ecs_sprites.h"
#include "ecs_physics.h"
#include "ecs_animations.h"
#include "tile_ids.h"

namespace map {
	unsigned int get_object_layer_index();
}

namespace ecs {
	extern entt::registry _registry;

	void update_pickups(float dt) {
		for (auto [entity, pickup] : _registry.view<Pickup>().each()) {
			pickup.timer.update(dt);
			if (pickup.timer.finished()) {
				destroy_at_end_of_frame(entity);
			}
		}

		for (auto [entity, pickup, sprite] : _registry.view<const Pickup, sprites::Sprite>().each()) {

			// Start blinking at >50% progress
			if (pickup.timer.get_progress() < 0.5f) continue;

			constexpr float BLINK_SPEED = 10.f;
			float blink_fraction = 0.75f + 0.25f * sin(pickup.timer.get_time_left() * BLINK_SPEED);
			sprite.color.a = (unsigned char)(255 * blink_fraction);
		}
	}

	entt::entity create_pickup(PickupType type, const Vector2f& position) {
		entt::entity entity = _registry.create();
		set_tag(entity, Tag::Pickup);
		{
			Pickup& pickup = emplace_pickup(entity);
			pickup.type = type;
			pickup.timer.start();
		}
		{
			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_staticBody;
			body_def.position = position;
			b2BodyId body = emplace_body(entity, body_def);
			b2ShapeDef shape_def = b2DefaultShapeDef();
			shape_def.isSensor = true;
			b2Circle circle{};
			circle.radius = 4.f;
			b2CreateCircleShape(body, &shape_def, &circle);
		}
		{
			TileAnimation& animation = emplace_tile_animation(entity);
			animation.tileset_id = get_tileset_id("items1");
			switch (type) {
			case PickupType::Arrow:
				animation.tile_id = TILE_ID_ITEM_SPEAR; // placeholder
				break;
			case PickupType::Rupee:
				animation.tile_id = TILE_ID_ITEM_RUPEE;
				break;
			case PickupType::Bomb:
				animation.tile_id = TILE_ID_ITEM_POTION; // placeholder
				break;
			case PickupType::Heart:
				animation.tile_id = TILE_ID_ITEM_BERRIES; // placeholder
				break;
			}

			sprites::Sprite& sprite = emplace_sprite(entity);
			sprite.texture = get_tileset_texture(animation.tileset_id);
			sprite.sorting_layer = (uint8_t)map::get_object_layer_index();
			sprite.sorting_point = { 8.f, 8.f };
			sprite.position = position - sprite.sorting_point;
			sprite.size = { 16.f, 16.f };
		}
		return entity;
	}

	Pickup& emplace_pickup(entt::entity entity) {
		return _registry.emplace_or_replace<Pickup>(entity);
	}

	Pickup* get_pickup(entt::entity entity) {
		return _registry.try_get<Pickup>(entity);
	}

	bool remove_pickup(entt::entity entity) {
		return _registry.remove<Pickup>(entity);
	}
}

