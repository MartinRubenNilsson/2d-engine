#pragma once
#include <unordered_set>

namespace ecs {
	struct Patch {
		std::unordered_set<entt::entity> entities_to_destroy;
		std::unordered_set<entt::entity> chests_to_open;

		auto operator<=>(const Patch&) const = default;
	};

	// Sets the map ID of the patch that get_patch() should return, WITHOUT creating it.
	// Returns true if the patch has been created before by calling get_patch().
	bool set_patch(MapId map);
	// Returns the patch with map ID set by set_patch(), creating it first if necessary.
	Patch& get_patch();
}