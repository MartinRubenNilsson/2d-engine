#include "stdafx.h"
#ifdef _DEBUG
#include "shapes.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"

namespace shapes {
	struct ViewBounds {
		float min_x = 0.f;
		float min_y = 0.f;
		float max_x = 0.f;
		float max_y = 0.f;
	};

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

	// Culling will use the view bounds from last render call,
	// leading to one frame of lag, but it's not a big deal.
	ViewBounds _last_calculated_view_bounds{};
	eastl::vector<Point> _points;
	eastl::vector<Line> _lines;
	eastl::vector<Box> _boxes;
	eastl::vector<Polygon> _polygons;
	eastl::vector<Circle> _circles;
	eastl::vector<Batch> _batches;

	bool _cull_point(const ViewBounds& bounds, const Vec2f& position) {
		if (position.x < bounds.min_x) return true;
		if (position.x > bounds.max_x) return true;
		if (position.y < bounds.min_y) return true;
		if (position.y > bounds.max_y) return true;
		return false;
	}

	bool _cull_line(const ViewBounds& bounds, const Vec2f& p1, const Vec2f& p2) {
		if (p1.x < bounds.min_x && p2.x < bounds.min_x) return true;
		if (p1.x > bounds.max_x && p2.x > bounds.max_x) return true;
		if (p1.y < bounds.min_y && p2.y < bounds.min_y) return true;
		if (p1.y > bounds.max_y && p2.y > bounds.max_y) return true;
		return false;
	}

	bool _cull_box(const ViewBounds& bounds, const Vec2f& min, const Vec2f& max) {
		if (max.x < bounds.min_x) return true;
		if (min.x > bounds.max_x) return true;
		if (max.y < bounds.min_y) return true;
		if (min.y > bounds.max_y) return true;
		return false;
	}

	bool _cull_polygon(const ViewBounds& bounds, const Vec2f* points, size_t count) {
		if (count < 3) return true;
		Vec2f min = points[0];
		Vec2f max = points[0];
		for (size_t i = 1; i < count; ++i) {
			min.x = std::min(min.x, points[i].x);
			min.y = std::min(min.y, points[i].y);
			max.x = std::max(max.x, points[i].x);
			max.y = std::max(max.y, points[i].y);
		}
		return _cull_box(bounds, min, max);
	}

	bool _cull_circle(const ViewBounds& bounds, const Vec2f& center, float radius) {
		if (center.x + radius < bounds.min_x) return true;
		if (center.x - radius > bounds.max_x) return true;
		if (center.y + radius < bounds.min_y) return true;
		if (center.y - radius > bounds.max_y) return true;
		return false;
	}

	void draw_point_later(const Vec2f& point, const Color& color, float lifetime) {
		if (lifetime <= 0.f && _cull_point(_last_calculated_view_bounds, point)) return;
		_points.emplace_back(point, color, lifetime);
	}

	void draw_line_later(const Vec2f& p1, const Vec2f& p2, const Color& color, float lifetime) {
		if (lifetime <= 0.f && _cull_line(_last_calculated_view_bounds, p1, p2)) return;
		_lines.emplace_back(p1, p2, color, lifetime);
	}

	void draw_box_later(const Vec2f& min, const Vec2f& max, const Color& color, float lifetime) {
		if (lifetime <= 0.f && _cull_box(_last_calculated_view_bounds, min, max)) return;
		_boxes.emplace_back(min, max, color, lifetime);
	}

	void draw_polygon_later(const Vec2f* points, unsigned int count, const Color& color, float lifetime) {
		count = std::min(count, MAX_POLYGON_VERTICES);
		if (count < 3) return;
		if (lifetime <= 0.f && _cull_polygon(_last_calculated_view_bounds, points, count)) return;
		Polygon& polygon = _polygons.emplace_back();
		memcpy(polygon.points, points, count * sizeof(Vec2f));
		polygon.count = count;
		polygon.color = color;
		polygon.lifetime = lifetime;
	}

	void draw_circle_later(const Vec2f& center, float radius, const Color& color, float lifetime) {
		if (lifetime <= 0.f && _cull_circle(_last_calculated_view_bounds, center, radius)) return;
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
		if (_points.empty()) return;
		Batch& batch = _batches.emplace_back();
		batch.primitive = graphics::Primitives::PointList;
		batch.vertex_offset = (unsigned int)graphics::temp_vertices.size();
		for (const Point& point : _points) {
			if (_cull_point(_last_calculated_view_bounds, point.position))
				continue;
			graphics::temp_vertices.emplace_back(point.position, point.color);
			batch.vertex_count += 1;
		}
	}

	void _create_line_batch() {
		if (_lines.empty()) return;
		Batch& batch = _batches.emplace_back();
		batch.primitive = graphics::Primitives::LineList;
		batch.vertex_offset = (unsigned int)graphics::temp_vertices.size();
		for (const Line& line : _lines) {
			if (_cull_line(_last_calculated_view_bounds, line.p1, line.p2))
				continue;
			graphics::temp_vertices.emplace_back(line.p1, line.color);
			graphics::temp_vertices.emplace_back(line.p2, line.color);
			batch.vertex_count += 2;
		}
	}

	void _create_box_batches() {
		for (const Box& box : _boxes) {
			if (_cull_box(_last_calculated_view_bounds, box.min, box.max)) continue;
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
			if (_cull_polygon(_last_calculated_view_bounds, polygon.points, polygon.count)) continue;
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
			if (_cull_circle(_last_calculated_view_bounds, circle.center, circle.radius)) continue;
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

	void _draw_all_batches(std::string_view debug_group_name) {
		graphics::ScopedDebugGroup debug_group(debug_group_name);

		const unsigned int vertices_byte_size = (unsigned int)graphics::temp_vertices.size() * sizeof(graphics::Vertex);
		if (vertices_byte_size <= graphics::get_buffer_size(graphics::dynamic_vertex_buffer)) {
			graphics::update_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices.data(), vertices_byte_size);
		} else {
			graphics::recreate_buffer(graphics::dynamic_vertex_buffer, vertices_byte_size, graphics::temp_vertices.data());
		}

		graphics::bind_vertex_buffer(0, graphics::dynamic_vertex_buffer, sizeof(graphics::Vertex));
		graphics::bind_vertex_shader(graphics::shape_vert);
		graphics::bind_fragment_shader(graphics::shape_frag);

		for (const Batch& draw : _batches) {
			graphics::set_primitives(draw.primitive);
			graphics::draw(draw.vertex_count, draw.vertex_offset);
		}
	}

	void draw_all_now(std::string_view debug_group_name, const Vec2f& camera_min, const Vec2f& camera_max) {

		_last_calculated_view_bounds.min_x = camera_min.x;
		_last_calculated_view_bounds.min_y = camera_min.y;
		_last_calculated_view_bounds.max_x = camera_max.x;
		_last_calculated_view_bounds.max_y = camera_max.y;

		_batches.clear();
		graphics::temp_vertices.clear();

		_create_point_batch();
		_create_line_batch();
		_create_box_batches();
		_create_polygon_batches();
		_create_circle_batches();

		if (_batches.empty())
			return; // nothing to draw

		_draw_all_batches(debug_group_name);

		_batches.clear();
		graphics::temp_vertices.clear();
	}
}

#endif // _DEBUG