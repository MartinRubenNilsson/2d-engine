#pragma once
#include "config.h"

#include <algorithm>
#include <bitset>
#include <format>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/vector.h>
#include <magic_enum/magic_enum.hpp>
#include <rapidjson/fwd.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <clay/clay.h>
#include <entt/entity/registry.hpp>
#include <box2d/box2d.h>

#include "fwd.h"
#include "handle.h"
#include "vec2.h"
#include "rect2.h"
#include "color.h"
#include "math.h"
#include "direction.h"
#include "text_ids.h"
#include "ecs_tiled_ids.h"
