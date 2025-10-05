#include "stdafx.h"
#include "ecs_ai.h"
#include "ecs_ai_action.h"
#include "ecs_animations.h"
#include "ecs_tiled.h"
#include "map_grid.h"
#include "random.h"
#include "shapes.h"
#include "text.h"
#include "fonts.h"

namespace ecs {
    extern entt::registry _registry;

    void _update_ai_decision_making(float dt) {
#if 0
        const bool player_exists = _registry.valid(world.player.entity);

        for (auto [entity, knowledge, type, action] :
            _registry.view<const AiType, const AiAction>().each()) {

            const float dist_to_player = length(world.player.position - knowledge.me.position);

            switch (type) {
                case AiType::None:
                    break; // Do nothing
                case AiType::Slime:
                {

                    if (action.type == AiActionType::Flee && action.status == AiActionStatus::Running) {
                    } else if (action.type == AiActionType::Pursue && action.status == AiActionStatus::Running) {
                    } else if (player_exists && dist_to_player < 25.f) {
                        ai_flee(entity, world.player.entity, knowledge.me.p_speed, 60.f);
                    } else if (player_exists && dist_to_player < 100.f) {
                        ai_pursue(entity, world.player.entity, knowledge.me.p_speed, 35.f, true);
                    } else if (action.type == AiActionType::Wait && action.status == AiActionStatus::Running) {
                    } else if (action.type == AiActionType::Wander && action.status == AiActionStatus::Succeeded) {
                        float duration = random::range_f(0.5f, 1.5f);
                        ai_wait(entity, duration);
                    }

                } break;
            }
        }
#endif
    }

    void update_ai_logic(float dt) {
        _update_ai_decision_making(dt);
        update_ai_actions(dt);
    }

    void emplace_ai(entt::entity entity, AiType type) {
        _registry.emplace_or_replace<AiType>(entity, type);
        _registry.emplace_or_replace<AiAction>(entity);
    }
}
