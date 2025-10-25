#pragma once
#include "color.h"
#include "vec2.h"

namespace graphics {
	struct VertexPC {
		Vec2f position;
		Color color;
	};

	struct VertexPCT {
		Vec2f position;
		Color color;
		Vec2f tex_coord;
	};
}