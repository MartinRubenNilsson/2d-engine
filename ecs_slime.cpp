#include "stdafx.h"
#include "ecs_slime.h"
#include "ecs_tags.h"
#include "ecs_damage.h"
#include "ecs_lifetime.h"
#include "ecs_tiled.h"
#include "ecs_animations.h"
#include "ecs_tasks.h"
#include "ecs_physics_events.h"
#include "audio.h"
#include "random.h"

namespace ecs {
    extern entt::registry _registry;

    bool _handle_damage_for_slime(entt::entity entity, const DamageEvent& ev) {
        audio::create_event({ .path = "event:/snd_slime_dying" });
        destroy_later(entity);
        return true;
    }

    void _handle_physics_for_slime(const TouchEvent& ev) {
        if (ev.type == TouchEventType::ContactBegin && get_tag(ev.other_entity) == Tag::Player) {
            deal_damage(ev.other_entity, {
                .type = DamageType::Touch,
                .amount = 1,
                .source = ev.entity });
        }
    }

    void _do_slime_task(entt::entity e) {
        //new_task("slime_task");
        wander(e, 20.f, 50.f, random::uniform_float(1.f, 3.f));
        then(e, [](entt::entity e) {
            wait(e, random::uniform_float(0.5f, 1.5f));
            then(e, [](entt::entity e) {
                pursue(e, find_entity_with_tag(Tag::Player), 30.f, 32.f);
                then(e, [](entt::entity e) {
                    flee(e, find_entity_with_tag(Tag::Player), 50.f, 16.f * 5);
                    then(e, _do_slime_task);
                });
            });
        });
        then(e, [](entt::entity e) {
            wait(e, 100.f);
        });
    }

    void setup_slimes() {
        for (auto [entity, object] : _registry.view<Type<Tag::Slime>, ObjectId>().each()) {
            _registry.emplace<Direction>(entity, Direction::S); // TODO: use this for something useful!
            set_touch_event_handler(entity, _handle_physics_for_slime);
            set_damage_event_handler(entity, _handle_damage_for_slime);
            _do_slime_task(entity);
        }
    }

    // slime.tsx
    enum TILE_ID_SLIME {
        TILE_ID_SLIME_WALK_S = 0,
        TILE_ID_SLIME_WALK_E = 4,
        TILE_ID_SLIME_WALK_N = 8,
        TILE_ID_SLIME_WALK_W = 12,
    };

    void update_slimes_graphics(float dt) {
        for (auto [entity, body, tile, anim] : _registry.view<Type<Tag::Slime>, b2BodyId, TileId, TileAnimation>().each()) {
            const Vec2f vel = b2Body_GetLinearVelocity(body);
            if (vel == Vec2f::ZERO) {
                anim.set_progress(0.f);
                continue;
            }
            switch (to_cardinal(vel)) {
                case Direction::S: replace(tile, TILE_ID_SLIME_WALK_S); break;
                case Direction::E: replace(tile, TILE_ID_SLIME_WALK_E); break;
                case Direction::N: replace(tile, TILE_ID_SLIME_WALK_N); break;
                case Direction::W: replace(tile, TILE_ID_SLIME_WALK_W); break;
            }
            const float speed = length(vel);
            const float anim_speed = speed / 16.f;
            anim.set_speed(anim_speed);
        }
    }
}