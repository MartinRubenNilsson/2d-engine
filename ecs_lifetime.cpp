#include "stdafx.h"
#include "ecs_lifetime.h"
#include "ecs_patch.h"

namespace ecs {
	extern entt::registry _registry;

	void destroy_now(entt::entity entity) {
		if (_registry.valid(entity)) {
			_registry.destroy(entity);
		}
	}

	void patch_entities_to_destroy(const Patch& patch) {
		for (entt::entity entity : patch.entities_to_destroy) {
			destroy_now(entity);
		}
	}

	std::unordered_set<entt::entity> _entities_to_destroy_later;

	void destroy_later(entt::entity entity) {
		if (_registry.valid(entity)) {
			_entities_to_destroy_later.insert(entity);
		}
	}

	void destroy_entities_to_be_destroyed_later() {
		for (entt::entity entity : _entities_to_destroy_later) {
			destroy_now(entity);
		}
		_entities_to_destroy_later.clear();
	}

	void clear_entities_to_be_destroyed_later() {
		_entities_to_destroy_later.clear();
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
				destroy_now(entity); // should this be destroy_later() just in case?
			}
		}
	}
}
