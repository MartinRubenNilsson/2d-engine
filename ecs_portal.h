#pragma once

namespace ecs {
	// Portals are used to teleport the player between maps.
	// They are defined in Tiled as objects with the "portal" class.

	struct Portal {
		std::string target_map; // The name of the map to which the player will be teleported.
		std::string target_point; // The name of the point to whose position the player will be teleported.
		std::string exit_direction; // The direction the player will be facing after teleporting.
		bool activated = false;
	};

	void setup_portals();
	void update_portals(float dt);

	// Returns the entity of the first active portal found, or entt::null if none exist.
	entt::entity find_active_portal_entity();

	Portal& emplace_portal(entt::entity entity, const Portal& portal = {});
	Portal* get_portal(entt::entity entity);
	bool remove_portal(entt::entity entity);
	bool has_portal(entt::entity entity);

	void activate_portal(entt::entity entity);
}