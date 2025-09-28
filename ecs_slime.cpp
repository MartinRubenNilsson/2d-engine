#include "stdafx.h"
#include "ecs_slime.h"
#include "ecs_tags.h"

namespace ecs {
    extern entt::registry _registry;

    void initialize_slimes() {
        for (auto [entity] : _registry.view<Type<Tag::Slime>>().each()) {

        }
    }

    void update_slimes(float dt) {
        // TODO
    }
}