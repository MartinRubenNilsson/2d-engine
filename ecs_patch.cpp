#include "stdafx.h"
#include "ecs_patch.h"

namespace ecs {
	std::unordered_map<std::string, Patch> _patches;
	std::string _current_patch_id;

	bool set_patch(const std::string& id) {
		_current_patch_id = id;
		return _patches.contains(id);
	}

	Patch& get_patch() {
		return _patches[_current_patch_id];
	}
}