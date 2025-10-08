#include "stdafx.h"
#ifdef _DEBUG_PHYSICS
#include "shapes.h"

namespace ecs {
	Color _to_color(b2HexColor hex_color) {
		Color color{};
		color.r = (hex_color >> 24) & 0xFF;
		color.g = (hex_color >> 16) & 0xFF;
		color.b = (hex_color >> 8) & 0xFF;
		color.a = 255;
		return color;
	}

	void _b2_debug_draw_polygon(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context) {
		shapes::draw_polygon_later((const Vec2f*)vertices, vertexCount, _to_color(color));
	}

	void _b2_debug_draw_solid_polygon(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context) {
		b2Vec2 transformed_vertices[B2_MAX_POLYGON_VERTICES];
		for (int i = 0; i < vertexCount; ++i) {
			transformed_vertices[i] = b2TransformPoint(transform, vertices[i]);
		}
		shapes::draw_polygon_later((const Vec2f*)transformed_vertices, vertexCount, _to_color(color));
	}

	void _b2_debug_draw_circle(b2Vec2 center, float radius, b2HexColor color, void* context) {
		shapes::draw_circle_later(center, radius, _to_color(color));
	}

	void _b2_debug_draw_solid_circle(b2Transform transform, float radius, b2HexColor color, void* context) {
		shapes::draw_circle_later(transform.p, radius, _to_color(color));
	}

	void _b2_debug_draw_solid_capsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context) {
		//TODO
	}

	void _b2_debug_draw_segment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context) {
		shapes::draw_line_later(p1, p2, _to_color(color));
	}

	void _b2_debug_draw_transform(b2Transform transform, void* context) {
		//TODO
	}

	void _b2_debug_draw_point(b2Vec2 p, float size, b2HexColor color, void* context) {
		//TODO
	}

	void _b2_debug_draw_string(b2Vec2 p, const char* s, b2HexColor color, void* context) {
		//TODO
	}

	extern b2WorldId _physics_world;

	void debug_draw_physics() {
		b2DebugDraw debug_draw{};
		debug_draw.DrawPolygonFcn = _b2_debug_draw_polygon;
		debug_draw.DrawSolidPolygonFcn = _b2_debug_draw_solid_polygon;
		debug_draw.DrawCircleFcn = _b2_debug_draw_circle;
		debug_draw.DrawSolidCircleFcn = _b2_debug_draw_solid_circle;
		debug_draw.DrawSolidCapsuleFcn = _b2_debug_draw_solid_capsule;
		debug_draw.DrawSegmentFcn = _b2_debug_draw_segment;
		debug_draw.DrawTransformFcn = _b2_debug_draw_transform;
		debug_draw.DrawPointFcn = _b2_debug_draw_point;
		debug_draw.DrawStringFcn = _b2_debug_draw_string;
		debug_draw.drawShapes = true;
		debug_draw.drawContacts = true;
		debug_draw.drawContactNormals = true;
		b2World_Draw(_physics_world, &debug_draw);
	}
}

#else

namespace ecs {
	void debug_draw_physics() {}
}

#endif