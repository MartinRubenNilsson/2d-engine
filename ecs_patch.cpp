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
	MapId _current_patch_id;

	bool set_patch(MapId map) {
		_current_patch_id = map;
		return _patches.contains(map);
	}

	Patch& get_patch() {
		return _patches[_current_patch_id];
	}
}