#include "stdafx.h"
#include "ecs_slime.h"
#include "ecs_tags.h"
#include "ecs_ai.h"
#include "ecs_damage.h"
#include "ecs_common.h"
#include "audio.h"

namespace ecs {
    extern entt::registry _registry;

    bool _handle_damage_to_slime(entt::entity entity, const Damage& damage) {
        //TODO: more stuff here

        audio::create_event({ .path = "event:/snd_slime_dying" });

        // TODO use snd_slime_hurt when slime is damaged and snd_slime_dying when slime is dead
        // audio::play("event:/snd_slime_hurt");

        destroy_at_end_of_frame(entity);
        return true;
    }

    void setup_slimes() {
        for (auto [entity] : _registry.view<Type<Tag::Slime>>().each()) {
            emplace_ai(entity, AiType::Slime);
            set_damage_handler(entity, _handle_damage_to_slime);
        }
    }
}