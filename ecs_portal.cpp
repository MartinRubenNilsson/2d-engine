#include "stdafx.h"
#include "ecs_portal.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"
#include "ecs_patch.h"
#include "ecs_physics_events.h"
#include "map.h"
#include "audio.h"

namespace ecs {
	struct Portal {
		std::string map; // The name of the map to which the player will be teleported.
		std::string portal; // The name of the exit portal to which player will be teleported.
		Vec2u direction; // The direction the player will exit the portal.
		bool activated = false;
	};

	extern entt::registry _registry;

	void _activate_portal(entt::entity entity) {
		Portal* portal = _registry.try_get<Portal>(entity);
		if (!portal) return;
		if (portal->activated) return;
		portal->activated = true;
		MapId map = get_map(portal->map);
		if (!map) return;
		if (!map::open(portal->map)) return;
		get_patch(map).portal_to_exit = portal->portal;
		audio::create_event({ .path = "event:/snd_map_transition" });
	}

	void _handle_physics_event_for_portal(const PhysicsEvent& ev) {
		if (ev.type != PhysicsEventType::SensorBeginTouch)
			return;
		if (get_tag(ev.other_entity) != Tag::Player)
			return;
		_activate_portal(ev.entity);
	}

	void setup_portals() {
		for (auto [entity, object] : _registry.view<Type<Tag::Portal>, ObjectId>().each()) {
			{
				Portal& portal = _registry.emplace<Portal>(entity);
				portal.map = get_string(object, "map");
				portal.portal = get_string(object, "portal");
				const std::string_view direction = get_string(object, "direction");
				if (direction == "left") {
					portal.direction.x = -1;
				} else if (direction == "right") {
					portal.direction.x = 1;
				} else if (direction == "up") {
					portal.direction.y = -1;
				} else if (direction == "down") {
					portal.direction.y = 1;
				}
			}
			set_physics_event_handler(entity, _handle_physics_event_for_portal);
		}
	}

	entt::entity get_portal_with_name(std::string_view name) {
		for (auto [entity, object] : _registry.view<Type<Tag::Portal>, ObjectId>().each()) {
			if (get_name(object) == name) {
				return entity;
			}
		}
		return entt::null;
	}

	bool teleport_entity_to_portal(entt::entity entity, entt::entity portal_entity) {
		b2BodyId* body = _registry.try_get<b2BodyId>(entity);
		if (!body) return false;
		b2BodyId* portal_body = _registry.try_get<b2BodyId>(portal_entity);
		if (!portal_body) return false;
		Portal* portal = _registry.try_get<Portal>(portal_entity);
		if (!portal) return false;
		// PITFALL: b2Body_GetWorldCenterOfMass() will not work since portals are sensors
		// and don't have mass data. Also b2Body_GetPosition() returns the top left corner.
		// Hence let's use the world AABB center
		Vec2f position = b2AABB_Center(b2Body_ComputeAABB(*portal_body));
		position += (Vec2f)portal->direction * 16.f;
		position = position - b2Body_GetLocalCenterOfMass(*body);
		b2Body_SetTransform(*body, position, b2Body_GetRotation(*body));
		return true;
	}
}
