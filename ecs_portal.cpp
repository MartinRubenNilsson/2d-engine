#include "stdafx.h"
#include "ecs_portal.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"
#include "map.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void setup_portals() {
		for (auto [entity, object] : _registry.view<Type<Tag::Portal>, TiledObject>().each()) {
			Portal& portal = _registry.emplace<Portal>(entity);
			portal.target_map = object.get_string("target_map");
			portal.target_point = object.get_string("target_point");
			portal.exit_direction = object.get_string("exit_direction");
		}
	}

	void update_portals(float dt) {
		//Empty
	}

	entt::entity find_active_portal_entity() {
		for (auto [entity, portal] : _registry.view<const Portal>().each()) {
			if (portal.activated) return entity;
		}
		return entt::null;
	}

	Portal& emplace_portal(entt::entity entity, const Portal& portal) {
		return _registry.emplace_or_replace<Portal>(entity, portal);
	}

	Portal* get_portal(entt::entity entity) {
		return _registry.try_get<Portal>(entity);
	}

	bool remove_portal(entt::entity entity) {
		return _registry.remove<Portal>(entity);
	}

	bool has_portal(entt::entity entity) {
		return _registry.all_of<Portal>(entity);
	}

	void activate_portal(entt::entity entity) {
		Portal* portal = get_portal(entity);
		if (!portal) return;
		if (portal->activated) return;
		portal->activated = true;
		if (!map::open(portal->target_map)) return;
		audio::create_event({ .path = "event:/snd_map_transition" });
	}
}
