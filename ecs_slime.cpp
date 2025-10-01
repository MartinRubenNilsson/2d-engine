#include "stdafx.h"
#include "ecs_slime.h"
#include "ecs_tags.h"
#include "ecs_ai.h"
#include "ecs_damage.h"
#include "ecs_lifetime.h"
#include "ecs_states.h"
#include "ecs_tiled.h"
#include "ecs_physics.h"
#include "audio.h"

namespace ecs {
    struct Slime {
        float speed = 0.f;
        entt::entity player_entity = entt::null;
        float distance_to_player = -1.f;
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

    void setup_slimes() {
        for (auto [entity, object] : _registry.view<Type<Tag::Slime>, TObject>().each()) {
            Slime& slime = _registry.emplace<Slime>(entity);
            slime.speed = object.get_float("speed");

            _emplace_state_machine_for_slime(entity);
            emplace_ai(entity, AiType::Slime);

            set_damage_handler(entity, _handle_damage_to_slime);
        }
    }

    void update_slimes(float dt) {
        const entt::entity player_entity = find_entity_by_tag(Tag::Player);
        const b2BodyId player_body = get_body(player_entity);
        const Vector2f player_pos = B2_IS_NON_NULL(player_body) ?
            (Vector2f)b2Body_GetWorldCenterOfMass(player_body) : Vector2f();

        for (auto [entity, slime, body] : _registry.view<Slime, b2BodyId>().each()) {
            Vector2f pos = b2Body_GetWorldCenterOfMass(body);
            slime.player_entity = player_entity;
            slime.distance_to_player = -1.f;
            if (player_entity != entt::null) {
                slime.distance_to_player = length(player_pos - pos);
            }
        }
    }
}