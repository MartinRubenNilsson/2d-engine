#pragma once
#include <unordered_set>

namespace ecs {
	struct Patch {
		MapId map{}; // The map being patched.
		Vec2f player_position = Vec2f::MAX;
		std::string portal_to_exit; // The name of the portal the player should exit.
		std::unordered_set<entt::entity> entities_to_destroy;
		std::unordered_set<entt::entity> chests_to_open;

		auto operator<=>(const Patch&) const = default;
	};

	extern MapId map_to_patch;

	// Returns true if get_patch() has been called for the given map.
	bool has_patch(MapId map = map_to_patch);
	// Returns the patch for the given map, creating it first if necessary.
	Patch& get_patch(MapId map = map_to_patch);
}