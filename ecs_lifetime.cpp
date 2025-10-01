#include "stdafx.h"
#include "ecs_lifetime.h"
#include "ecs_patch.h"

namespace ecs {
	extern entt::registry _registry;

	void destroy(entt::entity entity) {
		if (_registry.valid(entity)) {
			_registry.destroy(entity);
		}
	}

	void patch_entities_to_destroy(const Patch& patch) {
		for (entt::entity entity : patch.entities_to_destroy) {
			destroy(entity);
		}
	}

	std::unordered_set<entt::entity> _entities_to_destroy_at_end_of_frame;

	void destroy_at_end_of_frame(entt::entity entity) {
		if (_registry.valid(entity)) {
			_entities_to_destroy_at_end_of_frame.insert(entity);
		}
	}

	void destroy_entities_to_be_destroyed_at_end_of_frame() {
		for (entt::entity entity : _entities_to_destroy_at_end_of_frame) {
			destroy(entity);
		}
		_entities_to_destroy_at_end_of_frame.clear();
	}

	void clear_entities_to_destroy_at_end_of_frame() {
		_entities_to_destroy_at_end_of_frame.clear();
	}

	struct Lifetime {
		float time = 0.f;
	};

	void set_lifetime(entt::entity entity, float time) {
		_registry.emplace_or_replace<Lifetime>(entity, time);
	}

	void update_lifetimes(float dt) {
		for (auto [entity, lifetime] : _registry.view<Lifetime>().each()) {
			lifetime.time -= dt;
			if (lifetime.time <= 0.f) {
				destroy_at_end_of_frame(entity);
			}
		}
	}
}
