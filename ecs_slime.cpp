#include "stdafx.h"
#include "ecs_slime.h"
#include "ecs_tags.h"
#include "ecs_ai.h"
#include "ecs_damage.h"
#include "ecs_lifetime.h"
#include "ecs_states.h"
#include "ecs_tiled.h"
#include "ecs_animations.h"
#include "ecs_task.h"
#include "audio.h"
#include "random.h"

namespace ecs {
    struct Slime {
        float speed = 0.f;
    };

    extern entt::registry _registry;

    void _update_waiting_slime(entt::entity entity, float dt) {
        // TODO
    }

    void _update_wandering_slime(entt::entity entity, float dt) {
        // TODO
    }

    void _update_pursuing_slime(entt::entity entity, float dt) {
        // TODO
    }

    void _update_fleeing_slime(entt::entity entity, float dt) {
        // TODO
    }

    void _emplace_state_machine_for_slime(entt::entity entity) {
        StateMachine& sm = emplace_state_machine(entity);
        StateHandle waiting = add_state(sm, {
            .id = "waiting",
            .update = _update_wandering_slime });
        add_state(sm, {
            .id = "wandering",
            .update = _update_wandering_slime });
        add_state(sm, {
            .id = "pursuing",
            .update = _update_pursuing_slime });
        add_state(sm, {
            .id = "fleeing",
            .update = _update_fleeing_slime });
        transition(sm, waiting, entity);
    }

    bool _handle_damage_to_slime(entt::entity entity, const Damage& damage) {
        // Die immediately.
        audio::create_event({ .path = "event:/snd_slime_dying" });
        destroy_at_end_of_frame(entity);
        return true;
    }

    void _wander_then_wait_then_repeat(entt::entity entity) {
        wander(entity, 20.f, 50.f, random::range_f(1.f, 3.f));
        then(entity, [](entt::entity entity) {
            wait(entity, random::range_f(0.5f, 1.5f));
            then(entity, _wander_then_wait_then_repeat);
        });
    }

    void setup_slimes() {
        for (auto [entity, object] : _registry.view<Type<Tag::Slime>, ObjectId>().each()) {
            Slime& slime = _registry.emplace<Slime>(entity);
            slime.speed = get_float(object, "speed");

            set_damage_handler(entity, _handle_damage_to_slime);

            _wander_then_wait_then_repeat(entity);
            //_emplace_state_machine_for_slime(entity);
            //emplace_ai(entity, AiType::Slime);

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
        for (auto [entity, slime, body, animation] : _registry.view<Slime, b2BodyId, TileAnimation>().each()) {
            Vector2f velocity = b2Body_GetLinearVelocity(body);
            unsigned int tile_id = UINT_MAX;
            if (!is_zero(velocity)) {
                switch (get_direction(velocity)) {
                    case 'd': tile_id = TILE_ID_SLIME_WALK_DOWN; break;
                    case 'r': tile_id = TILE_ID_SLIME_WALK_RIGHT; break;
                    case 'u': tile_id = TILE_ID_SLIME_WALK_UP; break;
                    case 'l': tile_id = TILE_ID_SLIME_WALK_LEFT; break;
                }
            }
            animation.tile = change(animation.tile, tile_id);
            animation.speed = length(velocity) / 32.f;
        }
    }
}