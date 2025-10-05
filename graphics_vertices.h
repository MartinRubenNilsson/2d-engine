#pragma once
#include "color.h"
#include "vec2.h"

namespace graphics {
	struct Vertex {
		Vec2f position;
		Color color;
		Vec2f tex_coord;
	};
}