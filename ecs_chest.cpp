#include "stdafx.h"
#include "ecs_chest.h"
#include "ecs_tiled.h"
#include "ecs_bomb.h"
#include "ecs_physics.h"
#include "ecs_sprites.h"
#include "ecs_interactions.h"
#include "ecs_patch.h"
#include "ui_textbox.h"
#include "ecs_tags.h"
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

    void _open_chest(entt::entity entity, bool ignore_contents = false) {
        Chest* chest = _registry.try_get<Chest>(entity);
        if (!chest) return;
        if (chest->opened) return;
        chest->opened = true;

        if (TileId* tile = _registry.try_get<TileId>(entity)) {
            replace_step(*tile, 0, 1);
            if (sprites::Sprite* sprite = _registry.try_get<sprites::Sprite>(entity)) {
                update_sprite(*sprite, *tile, false);
            }
        }

        if (ignore_contents) return;

        switch (chest->type) {
        case ChestType::Normal: {
            ui::open_textbox({ .text = "You open the chest and find... nothing!" });
            get_patch().chests_to_open.insert(entity);
        } break;
        case ChestType::Bomb: {
            emplace_bomb(entity);
            ignite_bomb(entity);
            get_patch().entities_to_destroy.insert(entity);
        } break;
        }

        // Play opening sound.
        audio::create_event({ .path = "event:/props/chest/open" });
    }

    void _handle_interaction_with_chest(entt::entity entity) {
        _open_chest(entity);
    }

    void setup_chests() {
        for (auto [entity, object] : _registry.view<Type<Tag::Chest>, ObjectId>().each()) {

            const Vec2f top_left = get_top_left(object);
            const std::string_view type = get_string(object, "type");

            Chest& chest = _registry.emplace<Chest>(entity);
            if (type == "bomb") {
                chest.type = ChestType::Bomb;
            }

            const Vec2f pivot = { 16.f, 22.f };
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_staticBody;
                body_def.position = top_left + pivot;
                body_def.fixedRotation = true;
                b2BodyId body = emplace_body(entity, body_def);
                b2ShapeDef shape_def = b2DefaultShapeDef();
                b2Polygon box = b2MakeBox(10.f, 6.f);
                b2CreatePolygonShape(body, &shape_def, &box);
            }

            set_interaction_handler(entity, _handle_interaction_with_chest);
        }
    }

    void patch_chests(const Patch& patch) {
        for (entt::entity entity : patch.chests_to_open) {
            _open_chest(entity, true);
        }
    }
}