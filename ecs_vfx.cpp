#include "stdafx.h"
#include "ecs_vfx.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"
#include "ecs_lifetime.h"
#include "graphics.h"
#include "ecs_tiled.h"

namespace ecs {
    extern entt::registry _registry;

    entt::entity create_vfx(VfxType type, const Vector2f& position) {
        entt::entity entity = _registry.create();

        sprites::Sprite& sprite = emplace_sprite(entity);
        sprite.position = position;
        sprite.sorting_layer = get_object_layer();

        FlipbookAnimation& animation = emplace_flipbook_animation(entity);

        switch (type) {
            case VfxType::Explosion: {
                sprite.texture = graphics::load_texture("assets/textures/vfx/EXPLOSION.png");
                sprite.size.x = 109.f;
                sprite.size.y = 38.f;
                animation.rows = 1;
                animation.columns = 12;
                animation.fps = 20.f;
            } break;
        }

        sprite.position -= sprite.size / 2.f;
        sprite.sorting_point = sprite.size / 2.f;

        if (animation.fps > 0.f) {
            set_lifetime(entity, animation.rows * animation.columns / animation.fps);
        }

        return entity;
    }
}