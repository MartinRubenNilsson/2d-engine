#pragma once

namespace ecs {
    void emplace_bomb(entt::entity entity);
    void ignite_bomb(entt::entity entity);

    // Fails (returns entt::null) if the bomb would be created inside a wall.
    entt::entity create_bomb_at(const Vec2f& position);

    void update_bombs(float dt);
}