#include "stdafx.h"
#include "ecs_bomb.h"
#include "ecs_sprites.h"
#include "ecs_lifetime.h"
#include "ecs_camera.h"
#include "ecs_physics.h"
#include "ecs_physics_queries.h"
#include "ecs_vfx.h"
#include "ecs_damage.h"
#include "ecs_tiled.h"
#include "tile_ids.h"
#include "ecs_tags.h"
#include "audio.h"
#include "postprocessing.h"

namespace ecs {
    extern entt::registry _registry;

    void update_bombs(float dt) {
        for (auto [entity, bomb, body] : _registry.view<Bomb, b2BodyId>().each()) {

            if (!bomb.ignited) continue;

            const float progress_before = bomb.explosion_timer.get_progress();
            bomb.explosion_timer.update(dt);
            const float progress_after = bomb.explosion_timer.get_progress();

            if (progress_before < 0.5f && progress_after >= 0.5f) {
                make_sprite_blink(entity, {
                    .duration = bomb.explosion_timer.get_time_left(),
                    .interval = 0.2f,
                    .color = { 255, 0, 0, 255 } });
            }

            const Vec2f center = b2Body_GetPosition(body);
            audio::set_event_position(bomb.fuse_sound, center);

            if (!bomb.explosion_timer.finished()) continue;

            // Explode the bomb
            deal_damage_in_circle({ DamageType::Explosion, 2, entity }, center, bomb.explosion_radius);
            add_trauma_to_active_camera(0.8f);
            create_vfx(VfxType::Explosion, center);
            destroy_later(entity);
            audio::create_event({ .path = "event:/snd_bomb_explosion", .position = center });
            audio::stop_event(bomb.fuse_sound);
            postprocessing::add_shockwave(center);
        }
    }

    Bomb& emplace_bomb(entt::entity entity, const Bomb& bomb) {
        return _registry.emplace_or_replace<Bomb>(entity, bomb);
    }

    Bomb* get_bomb(entt::entity entity) {
        return _registry.try_get<Bomb>(entity);
    }

    entt::entity create_bomb(const Vec2f& position) {
        if (!overlap_circle(position, 4.f).empty()) {
            return entt::null;
        }

        entt::entity entity = _registry.create();
        set_tag(entity, Tag::Bomb);
        emplace_bomb(entity);
        ignite_bomb(entity);
        if (const TilesetId tileset = get_tileset("items1")) {
            if (const TileId tile = get_tile(tileset, TILE_ID_ITEM_POTION)) { // placeholder
                sprites::Sprite& sprite = emplace_sprite(entity);
                setup_sprite(sprite, tile, true);
                sprite.sorting_layer = get_object_layer();
                sprite.sorting_point = { 8.f, 16.f };
                sprite.position = position - sprite.sorting_point;
            }
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

    void ignite_bomb(entt::entity entity) {
        Bomb* bomb = get_bomb(entity);
        if (!bomb) return;
        if (bomb->ignited) return;
        bomb->ignited = true;
        bomb->explosion_timer.start();
        bomb->fuse_sound = audio::create_event({ .path = "event:/snd_bomb_fuse" });
    }

    bool apply_damage_to_bomb(entt::entity entity, const DamageEvent& ev) {
        // DON'T call _explode_bomb() directly here, I got a stack overflow
        // when two bombs kept on exploding each other in an infinite loop!
        if (ev.amount <= 0) return false;
        Bomb* bomb = get_bomb(entity);
        if (!bomb) return false;
        bomb->explosion_timer.finish();
        return true;
    }
}
