#include "stdafx.h"
#include "ecs_bomb.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"
#include "ecs_common.h"
#include "ecs_camera.h"
#include "ecs_physics.h"
#include "ecs_vfx.h"
#include "ecs_damage.h"
#include "tile_ids.h"
#include "audio.h"
#include "postprocessing.h"

namespace map
{
    unsigned int get_object_layer_index();
}

namespace ecs
{
    extern entt::registry _registry;

    void update_bombs(float dt)
    {
        for (auto [entity, bomb, body] : _registry.view<Bomb, b2BodyId>().each()) {

            if (!bomb.ignited) continue;

			const float progress_before = bomb.explosion_timer.get_progress();
            bomb.explosion_timer.update(dt);
			const float progress_after = bomb.explosion_timer.get_progress();

			if (progress_before < 0.5f && progress_after >= 0.5f) {
                SpriteBlink& blink = emplace_sprite_blink(entity);
				blink.duration = bomb.explosion_timer.get_time_left();
				blink.interval = 0.2f;
				blink.color = { 255, 0, 0, 255 };
			}

            const Vector2f center = b2Body_GetPosition(body);
            audio::set_event_position(bomb.fuse_sound, center);

            if (!bomb.explosion_timer.finished()) continue;

            // Explode the bomb
            apply_damage_in_circle({ DamageType::Explosion, 2, entity }, center, bomb.explosion_radius);
            add_trauma_to_active_camera(0.8f);
            create_vfx(VfxType::Explosion, center);
            destroy_at_end_of_frame(entity);
            audio::create_event({ .path = "event:/snd_bomb_explosion", .position = center });
            audio::stop_event(bomb.fuse_sound);
            //postprocessing::add_shockwave(center);
		}
    }

    Bomb& emplace_bomb(entt::entity entity, const Bomb& bomb)
    {
        return _registry.emplace_or_replace<Bomb>(entity, bomb);
    }

    Bomb* get_bomb(entt::entity entity)
    {
        return _registry.try_get<Bomb>(entity);
    }

    entt::entity create_bomb(const Vector2f& position)
    {
        if (!overlap_circle(position, 4.f).empty()) {
			return entt::null;
        }

        entt::entity entity = _registry.create();
        set_tag(entity, Tag::Bomb);
        emplace_bomb(entity);
        ignite_bomb(entity);
        {
			sprites::Sprite& sprite = emplace_sprite(entity);
            sprite.sorting_layer = map::get_object_layer_index();
            sprite.sorting_point = { 8.f, 16.f };
            sprite.position = position - sprite.sorting_point;
			sprite.size = { 16.f, 16.f };
        }
        {
            TileAnimation& animation = emplace_tile_animation(entity);
            animation.tileset = get_tileset("items1");
            animation.tile_id = TILE_ID_ITEM_POTION; // placeholder
        }
        {
            b2BodyDef body_def = b2DefaultBodyDef();
            body_def.type = b2_staticBody;
            body_def.position = position;
            body_def.fixedRotation = true;
            b2BodyId body = emplace_body(entity, body_def);
            b2ShapeDef shape_def = b2DefaultShapeDef();
            b2Circle circle{};
			circle.radius = 4.f;
			b2CreateCircleShape(body, &shape_def, &circle);
        }
        return entity;
    }

    void ignite_bomb(entt::entity entity)
    {
        Bomb* bomb = get_bomb(entity);
		if (!bomb) return;
		if (bomb->ignited) return;
		bomb->ignited = true;
        bomb->explosion_timer.start();
		bomb->fuse_sound = audio::create_event({ .path = "event:/snd_bomb_fuse" });
    }

    bool apply_damage_to_bomb(entt::entity entity, const Damage& damage)
    {
        // DON'T call _explode_bomb() directly here, I got a stack overflow
        // when two bombs kept on exploding each other in an infinite loop!
        if (damage.amount <= 0) return false;
        Bomb* bomb = get_bomb(entity);
        if (!bomb) return false;
        bomb->explosion_timer.finish();
        return true;
    }
}
