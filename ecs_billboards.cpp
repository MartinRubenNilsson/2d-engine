#include "stdafx.h"
#include "ecs_billboards.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"
#include "ecs_interactions.h"
#include "ecs_physics.h"
#include "ecs_sprites.h"
#include "ecs_damage.h"
#include "ecs_lifetime.h"
#include "ui_textboxes.h"

namespace ecs {
	struct Billboard {
		std::string textbox_path{};
	};

	extern entt::registry _registry;

	void _handle_interaction_with_billboard(entt::entity entity, const InteractionEvent& ev) {
        if (ev.source_dir != Direction::N)
            return; // Can only read the billboard when facing north.
        Billboard& billboard = _registry.get<Billboard>(entity);
        if (billboard.textbox_path.empty())
            return;
        ui::textboxes::open_next(billboard.textbox_path);
	}

    bool _handle_damage_to_billboard(entt::entity entity, const DamageEvent& ev) {
        destroy_later(entity);
        return true;
    }

	void setup_billboards() {
		for (auto [entity, sprite, object] : _registry.view<Type<Tag::Billboard>, sprites::Sprite, ObjectId>().each()) {
            constexpr Vec2f pivot = { 8.f, 16.f };
            sprite.sorting_point = pivot;
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_staticBody;
                body_def.position = get_top_left(object) + pivot;
                body_def.fixedRotation = true;
                b2BodyId body = emplace_body(entity, body_def);
                b2ShapeDef shape_def = b2DefaultShapeDef();
                shape_def.enableContactEvents = true;
                b2Polygon box = b2MakeBox(4.f, 2.f);
                b2CreatePolygonShape(body, &shape_def, &box);
            }
            set_interaction_event_handler(entity, _handle_interaction_with_billboard);
            set_damage_event_handler(entity, _handle_damage_to_billboard);
            const std::string_view textbox_path = get_string(object, "textbox");
            _registry.emplace<Billboard>(entity, std::string(textbox_path));
		}
	}
}