#include "stdafx.h"
#include "ecs_slime.h"
#include "ecs_tags.h"
#include "ecs_damage.h"
#include "ecs_lifetime.h"
#include "ecs_tiled.h"
#include "ecs_animations.h"
#include "ecs_tasks.h"
#include "audio.h"
#include "random.h"

namespace ecs {
    extern entt::registry _registry;

    bool _handle_damage_to_slime(entt::entity entity, const Damage& damage) {
        // Die immediately.
        audio::create_event({ .path = "event:/snd_slime_dying" });
        destroy_at_end_of_frame(entity);
        return true;
    }

    void _do_slime_task(entt::entity e) {
        wander(e, 20.f, 50.f, random::range_f(1.f, 3.f));
        then(e, [](entt::entity e) {
            wait(e, random::range_f(0.5f, 1.5f));
            then(e, [](entt::entity e) {
                pursue(e, find_entity_with_tag(Tag::Player), 30.f, 32.f);
                then(e, [](entt::entity e) {
                    flee(e, find_entity_with_tag(Tag::Player), 50.f, 16.f * 5);
                    then(e, _do_slime_task);
                });
            });
        });
    }

    void setup_slimes() {
        for (auto [entity, object] : _registry.view<Type<Tag::Slime>, ObjectId>().each()) {
            set_damage_handler(entity, _handle_damage_to_slime);
            _do_slime_task(entity);
        }
    }

    // slime.tsx
    enum TILE_ID_SLIME {
        TILE_ID_SLIME_WALK_DOWN = 0,
        TILE_ID_SLIME_WALK_RIGHT = 4,
        TILE_ID_SLIME_WALK_UP = 8,
        TILE_ID_SLIME_WALK_LEFT = 12,
    };

    void update_slimes(float dt) {
        for (auto [entity, body, tile, anim] : _registry.view<Type<Tag::Slime>, b2BodyId, TileId, TileAnimation>().each()) {
            Vec2f velocity = b2Body_GetLinearVelocity(body);
            unsigned int tile_id = UINT_MAX;
            if (velocity != Vec2f::ZERO) {
                switch (get_direction(velocity)) {
                    case 'd': tile_id = TILE_ID_SLIME_WALK_DOWN; break;
                    case 'r': tile_id = TILE_ID_SLIME_WALK_RIGHT; break;
                    case 'u': tile_id = TILE_ID_SLIME_WALK_UP; break;
                    case 'l': tile_id = TILE_ID_SLIME_WALK_LEFT; break;
                }
            }
            replace(tile, tile_id);
            anim.set_speed(length(velocity) / 32.f);
        }
    }
}