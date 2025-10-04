#include "stdafx.h"
#include "ecs_patch.h"

namespace std {
	template <>
	struct hash<ecs::MapId> {
		size_t operator()(const ecs::MapId& map) const {
			return (size_t)map.id;
		}
	};
}

namespace ecs {
	std::unordered_map<MapId, Patch> _patches;
	MapId map_to_patch;

	bool has_patch(MapId map) {
		return _patches.contains(map);
	}

	Patch& get_patch(MapId map) {
		return _patches.try_emplace(map).first->second;
	}
}