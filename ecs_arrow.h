#pragma once

namespace ecs {

    struct PhysicsEvent;

    struct Arrow {
        int damage = 0;
        float lifetime = 0.f; // unused right now
    };

    Arrow& emplace_arrow(entt::entity entity, const Arrow& arrow = {});

    entt::entity create_arrow(const Vec2f& position, const Vec2f& velocity);
}