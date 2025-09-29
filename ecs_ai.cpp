#include "stdafx.h"
#include "ecs_ai.h"
#include "ecs_ai_knowledge.h"
#include "ecs_ai_action.h"
#include "ecs_animations.h"
#include "ecs_common.h"
#include "map_grid.h"
#include "random.h"
#include "audio.h"
#include "shapes.h"
#include "text.h"
#include "fonts.h"
#include "tile_ids.h"

namespace ecs {
    extern entt::registry _registry;

    void _update_ai_decision_making(float dt) {
        const AiWorld& world = get_ai_world();
        const bool player_exists = _registry.valid(world.player.entity);

        for (auto [entity, knowledge, type, action] :
            _registry.view<const AiKnowledge, const AiType, const AiAction>().each()) {

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
                } else if (action.type != AiActionType::Wander) {
                    float duration = random::range_f(1.f, 3.f);
                    ai_wander(entity, knowledge.initial_position, 20.f, 50.f, duration);
                }

            } break;
            }
        }
    }

    void update_ai_logic(float dt) {
        update_ai_knowledge_and_world(dt);
        _update_ai_decision_making(dt);
        update_ai_actions(dt);
    }

    void update_ai_graphics(float dt) {
        for (auto [entity, animation, ai_type, body] :
            _registry.view<TileAnimation, const AiType, b2BodyId>().each()) {
            switch (ai_type) {
            case AiType::Slime:
            {
                Vector2f velocity = b2Body_GetLinearVelocity(body);
                if (!is_zero(velocity)) {
                    switch (get_direction(velocity)) {
                    case 'd': animation.tile_id = TILE_ID_SLIME_WALK_DOWN; break;
                    case 'r': animation.tile_id = TILE_ID_SLIME_WALK_RIGHT; break;
                    case 'u': animation.tile_id = TILE_ID_SLIME_WALK_UP; break;
                    case 'l': animation.tile_id = TILE_ID_SLIME_WALK_LEFT; break;
                    }
                }
                animation.speed = length(velocity) / 32.f;
            } break;
            }
        }
    }

    void debug_draw_ai() {
#ifdef _DEBUG
        text::Text text{};
        text.font = fonts::load_font("assets/fonts/Helvetica.ttf");;
        text.pixel_height = 48.f;
        text.scale = { 0.1f, 0.1f };

        uint32_t paths_drawn = 0;

        for (auto [entity, knowledge, action] : _registry.view<const AiKnowledge, const AiAction>().each()) {

            // DRAW ACTION TYPE
            {
                const std::string action_type_string = to_string(action.type);
                text.string.assign(action_type_string.begin(), action_type_string.end());
                text.position = knowledge.me.position + Vector2f(-8.f, -10.f);
                text::render(text);
            }

            // DRAW ACTION PATH

            if (action.path.size() >= 2) {
                for (size_t i = 0; i + 1 < action.path.size(); ++i) {
                    Vector2f p1 = map::get_tile_center(action.path[i]);
                    Vector2f p2 = map::get_tile_center(action.path[i + 1]);
                    Color color = random::color((uint32_t)entity * 2);
                    p1.x += paths_drawn * 2.f;
                    p1.y += paths_drawn * 2.f;
                    p2.x += paths_drawn * 2.f;
                    p2.y += paths_drawn * 2.f;
                    shapes::add_line(p1, p2, color);
                }
                ++paths_drawn;
            }
        }
#endif
    }

    void emplace_ai(entt::entity entity, AiType type) {
        emplace_ai_knowledge(entity);
        _registry.emplace_or_replace<AiType>(entity, type);
        _registry.emplace_or_replace<AiAction>(entity);
    }
}
