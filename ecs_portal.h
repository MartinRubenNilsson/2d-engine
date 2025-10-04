#pragma once

namespace ecs {
	void setup_portals();

	entt::entity get_portal_with_name(std::string_view name);
	bool teleport_entity_to_portal(entt::entity entity, entt::entity portal_entity);
}