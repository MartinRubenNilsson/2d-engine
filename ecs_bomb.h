#pragma once
#include "timer.h"

namespace ecs {
    struct Bomb {
        bool ignited = false;
        Timer explosion_timer = { 3.f };
        float explosion_radius = 20.f;
        Handle<audio::Event> fuse_sound;
    };

    void update_bombs(float dt);

    Bomb& emplace_bomb(entt::entity entity, const Bomb& bomb = {});
    Bomb* get_bomb(entt::entity entity);

    // Fails (returns entt::null) if the bomb would be created inside a wall.
    entt::entity create_bomb(const Vec2f& position);

    void ignite_bomb(entt::entity entity);
}