#include "stdafx.h"
#include "ecs_chest.h"
#include "ecs_tiled.h"
#include "ecs_animations.h"
#include "ecs_bomb.h"
#include "ecs_physics.h"
#include "ui_textbox.h"
#include "ecs_interactions.h"
#include "map.h"
#include "audio.h"

namespace ecs {
    enum class ChestType {
        Normal,
        Bomb, // trap chest; explodes when opened
    };

    struct Chest {
        ChestType type = ChestType::Normal;
        bool opened = false;
    };

    extern entt::registry _registry;

    void open_chest(entt::entity entity, bool ignore_contents) {
        Chest* chest = _registry.try_get<Chest>(entity);
        if (!chest) return;
        if (chest->opened) return;
        chest->opened = true;

        if (TileAnimation* animation = get_tile_animation(entity)) {
            // At the time I'm writing this code, the treasure chest tileset has 6 rows and 5 columns. 
            // Each chest's closed sprite is on an even row and its corresponding open sprite is right below it.
            constexpr unsigned int COLUMNS = 5;
            unsigned int x = animation->tile_id / COLUMNS;
            unsigned int y = animation->tile_id % COLUMNS;
            if (x % 2 == 0) {
                // If the chest is closed, we open it by setting the tile to the one right below it.
                animation->tile_id = (x + 1) * COLUMNS + y;
            }
        }

        if (ignore_contents) return;

        switch (chest->type) {
        case ChestType::Normal: {
            ui::open_textbox({ .text = "You open the chest and find... nothing!" });
            // By marking the chest as opened, we ensure that the chest
            // stays open when the player re-enters the map.
            map::mark_chest_as_opened(entity);
        } break;
        case ChestType::Bomb: {
            emplace_bomb(entity);
            ignite_bomb(entity);
            // By marking the entity as destroyed, we ensure that the chest
            // stays removed from the map when the player re-enters the map.
            map::mark_entity_as_destroyed(entity);
        } break;
        }

        audio::create_event({ .path = "event:/props/chest/open" });
    }

    void _handle_interaction_with_chest(entt::entity entity) {
        open_chest(entity);
    }

    void setup_chests() {
        for (auto [entity, object] : _registry.view<Type<Tag::Chest>, TiledObject>().each()) {

            const Vector2f top_left = object.get_top_left();
            const std::string_view chest_type = object.get_string("type");

            Chest& chest = _registry.emplace<Chest>(entity);
            if (chest_type == "bomb") {
                chest.type = ChestType::Bomb;
            }

            const Vector2f pivot = { 16.f, 22.f };
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_staticBody;
                body_def.position = top_left + pivot;
                body_def.fixedRotation = true;
                b2BodyId body = ecs::emplace_body(entity, body_def);
                b2ShapeDef shape_def = b2DefaultShapeDef();
                b2Polygon box = b2MakeBox(10.f, 6.f);
                b2CreatePolygonShape(body, &shape_def, &box);
            }

            ecs::set_interaction_handler(entity, ecs::_handle_interaction_with_chest);
        }
    }
}