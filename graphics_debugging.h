#pragma once
#include <string_view>

namespace graphics {
	void push_debug_group(std::string_view name);
	void pop_debug_group();

	struct ScopedDebugGroup {
		ScopedDebugGroup(std::string_view name) { push_debug_group(name); }
		~ScopedDebugGroup() { pop_debug_group(); }
	};

#define GRAPHICS_DEBUG_GROUP const graphics::ScopedDebugGroup _scoped_debug_group(__FUNCTION__)
}