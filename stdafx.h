#pragma once
#include "config.h"

#include <algorithm>
#include <format>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#define EASTL_USER_DEFINED_ALLOCATOR

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/vector.h>

using namespace std::literals::string_literals;

#include <entt/entity/registry.hpp>
#include <box2d/box2d.h>
#include <RmlUi/Core.h> // TODO: get rid of this crap!!!
#include <clay/clay.h>
#include <imgui.h>
#include <imgui_stdlib.h>

#include <magic_enum/magic_enum.hpp>

#include "fwd.h"
#include "handle.h"
#include "vec2.h"
#include "rect2.h"
#include "color.h"
#include "math.h"
#include "direction.h"
#include "ecs_tiled_ids.h"
