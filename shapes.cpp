#include "stdafx.h"
#include "shapes.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"
#include "graphics_debugging.h"

namespace shapes {
	struct Point {
		Vec2f position;
		Color color = Color::WHITE;
		float lifetime = 0.f;
	};

	struct Line {
		Vec2f p1;
		Vec2f p2;
		Color color = Color::WHITE;
		float lifetime = 0.f;
	};

	struct Box {
		Vec2f min;
		Vec2f max;
		Color color = Color::WHITE;
		float lifetime = 0.f;
	};

	struct Polygon {
		Vec2f points[MAX_POLYGON_VERTICES];
		unsigned int count = 0;
		Color color = Color::WHITE;
		float lifetime = 0.f;
	};

	struct Circle {
		Vec2f center;
		float radius = 0.f;
		Color color = Color::WHITE;
		float lifetime = 0.f;
	};

	struct Batch {
		graphics::Primitives primitive{};
		unsigned int vertex_count = 0;
		unsigned int vertex_offset = 0;
	};

	eastl::vector<Point> _points;
	eastl::vector<Line> _lines;
	eastl::vector<Box> _boxes;
	eastl::vector<Polygon> _polygons;
	eastl::vector<Circle> _circles;
	eastl::vector<Batch> _batches;


	bool _cull_point(const Rect2f& view, const Vec2f& position) {
		if (position.x < view.min.x) return true;
		if (position.x > view.max.x) return true;
		if (position.y < view.min.y) return true;
		if (position.y > view.max.y) return true;
		return false;
	}

	bool _cull_line(const Rect2f& view, const Vec2f& p1, const Vec2f& p2) {
		if (p1.x < view.min.x && p2.x < view.min.x) return true;
		if (p1.x > view.max.x && p2.x > view.max.x) return true;
		if (p1.y < view.min.y && p2.y < view.min.y) return true;
		if (p1.y > view.max.y && p2.y > view.max.y) return true;
		return false;
	}

	bool _cull_box(const Rect2f& view, const Vec2f& min, const Vec2f& max) {
		if (max.x < view.min.x) return true;
		if (min.x > view.max.x) return true;
		if (max.y < view.min.y) return true;
		if (min.y > view.max.y) return true;
		return false;
	}

	bool _cull_polygon(const Rect2f& view, const Vec2f* points, size_t count) {
		if (count < 3) return true;
		Vec2f min = points[0];
		Vec2f max = points[0];
		for (size_t i = 1; i < count; ++i) {
			min.x = std::min(min.x, points[i].x);
			min.y = std::min(min.y, points[i].y);
			max.x = std::max(max.x, points[i].x);
			max.y = std::max(max.y, points[i].y);
		}
		return _cull_box(view, min, max);
	}

	bool _cull_circle(const Rect2f& view, const Vec2f& center, float radius) {
		if (center.x + radius < view.min.x) return true;
		if (center.x - radius > view.max.x) return true;
		if (center.y + radius < view.min.y) return true;
		if (center.y - radius > view.max.y) return true;
		return false;
	}

	void draw_point_later(const Vec2f& point, const Color& color, float lifetime) {
		if (lifetime < 0.f) return;
		_points.emplace_back(point, color, lifetime);
	}

	void draw_line_later(const Vec2f& p1, const Vec2f& p2, const Color& color, float lifetime) {
		if (lifetime < 0.f) return;
		_lines.emplace_back(p1, p2, color, lifetime);
	}

	void draw_box_later(const Vec2f& min, const Vec2f& max, const Color& color, float lifetime) {
		if (lifetime < 0.f) return;
		_boxes.emplace_back(min, max, color, lifetime);
	}

	void draw_polygon_later(const Vec2f* points, unsigned int count, const Color& color, float lifetime) {
		count = std::min(count, MAX_POLYGON_VERTICES);
		if (count < 3) return;
		if (lifetime < 0.f) return;
		Polygon& polygon = _polygons.emplace_back();
		memcpy(polygon.points, points, count * sizeof(Vec2f));
		polygon.count = count;
		polygon.color = color;
		polygon.lifetime = lifetime;
	}

	void draw_circle_later(const Vec2f& center, float radius, const Color& color, float lifetime) {
		if (lifetime < 0.f) return;
		_circles.emplace_back(center, radius, color, lifetime);
	}

	template <typename T>
	void _update_lifetimes(eastl::vector<T>& vec, float dt) {
		size_t size = vec.size();
		for (size_t i = size; i--;) {
			vec[i].lifetime -= dt;
			if (vec[i].lifetime > 0.f) continue;
			--size;
			if (i == size) continue;
			if constexpr (std::is_trivially_copyable_v<T>) {
				memcpy(&vec[i], &vec[size], sizeof(T));
			} else {
				vec[i] = std::move(vec[size]);
			}
		}
		vec.resize(size);
	}

	void update_lifetimes(float dt) {
		_update_lifetimes(_points, dt);
		_update_lifetimes(_lines, dt);
		_update_lifetimes(_boxes, dt);
		_update_lifetimes(_polygons, dt);
		_update_lifetimes(_circles, dt);
	}

	void _create_point_batch() {
		if (_points.empty())
			return;
		Batch& batch = _batches.emplace_back();
		batch.primitive = graphics::Primitives::PointList;
		batch.vertex_offset = (unsigned int)graphics::temp_vertices.size();
		for (const Point& point : _points) {
			graphics::temp_vertices.emplace_back(point.position, point.color);
			batch.vertex_count += 1;
		}
	}

	void _create_line_batch() {
		if (_lines.empty())
			return;
		Batch& batch = _batches.emplace_back();
		batch.primitive = graphics::Primitives::LineList;
		batch.vertex_offset = (unsigned int)graphics::temp_vertices.size();
		for (const Line& line : _lines) {
			graphics::temp_vertices.emplace_back(line.p1, line.color);
			graphics::temp_vertices.emplace_back(line.p2, line.color);
			batch.vertex_count += 2;
		}
	}

	void _create_box_batches() {
		for (const Box& box : _boxes) {
			Batch& draw = _batches.emplace_back();
			draw.primitive = graphics::Primitives::LineStrip;
			draw.vertex_count = 5;
			draw.vertex_offset = (unsigned int)graphics::temp_vertices.size();
			graphics::temp_vertices.emplace_back(Vec2f{ box.min.x, box.min.y }, box.color);
			graphics::temp_vertices.emplace_back(Vec2f{ box.max.x, box.min.y }, box.color);
			graphics::temp_vertices.emplace_back(Vec2f{ box.max.x, box.max.y }, box.color);
			graphics::temp_vertices.emplace_back(Vec2f{ box.min.x, box.max.y }, box.color);
			graphics::temp_vertices.emplace_back(graphics::temp_vertices[draw.vertex_offset]);
		}
	}

	void _create_polygon_batches() {
		for (const Polygon& polygon : _polygons) {
			Batch& draw = _batches.emplace_back();
			draw.primitive = graphics::Primitives::LineStrip;
			draw.vertex_count = polygon.count + 1;
			draw.vertex_offset = (unsigned int)graphics::temp_vertices.size();
			for (unsigned int i = 0; i < polygon.count; ++i) {
				graphics::temp_vertices.emplace_back(polygon.points[i], polygon.color);
			}
			graphics::temp_vertices.emplace_back(graphics::temp_vertices[draw.vertex_offset]);
		}
	}

	void _create_circle_batches() {
		for (const Circle& circle : _circles) {
			constexpr unsigned int SUBDIVISIONS = 32;
			constexpr float ANGLE_STEP = 6.283185307f / SUBDIVISIONS;
			Batch& draw = _batches.emplace_back();
			draw.primitive = graphics::Primitives::LineStrip;
			draw.vertex_count = SUBDIVISIONS + 1;
			draw.vertex_offset = (unsigned int)graphics::temp_vertices.size();
			for (unsigned int i = 0; i < SUBDIVISIONS; ++i) {
				const float angle = i * ANGLE_STEP;
				const Vec2f position = circle.center + circle.radius * Vec2f{ cos(angle), sin(angle) };
				graphics::temp_vertices.emplace_back(position, circle.color);
			}
			graphics::temp_vertices.emplace_back(graphics::temp_vertices[draw.vertex_offset]);
		}
	}

	void draw_all_now() {
		_batches.clear();
		graphics::temp_vertices.clear();

		_create_point_batch();
		_create_line_batch();
		_create_box_batches();
		_create_polygon_batches();
		_create_circle_batches();

		if (_batches.empty())
			return; // nothing to draw

		GRAPHICS_DEBUG_GROUP;

		graphics::update_or_recreate_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices.data(),
			(unsigned int)graphics::temp_vertices.size() * sizeof(graphics::Vertex));
		graphics::bind_vertex_buffer(0, graphics::dynamic_vertex_buffer, sizeof(graphics::Vertex));
		graphics::bind_vertex_shader(graphics::shape_vert);
		graphics::bind_fragment_shader(graphics::shape_frag);
		for (const Batch& draw : _batches) {
			graphics::set_primitives(draw.primitive);
			graphics::draw(draw.vertex_count, draw.vertex_offset);
		}

		_batches.clear();
		graphics::temp_vertices.clear();
	}
}