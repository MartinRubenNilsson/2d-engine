#include "stdafx.h"
#include "math.h"

float smoothstep(float x) {
	return x * x * (3.f - 2.f * x);
}

float smootherstep(float x) {
	return x * x * x * (x * (x * 6.f - 15.f) + 10.f);
}

float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

float lerp_angle(float a, float b, float t) {
	float angle = fmod(b - a, M_2PI);
	if (angle > M_PI) angle -= M_2PI;
	else if (angle < -M_PI) angle += M_2PI;
	return a + angle * t;
}

float length_squared(const Vec2f& v) {
	return v.x * v.x + v.y * v.y;
}

float length(const Vec2f& v) {
	return sqrt(length_squared(v));
}

Vec2f unit(float angle) {
	return { cos(angle), sin(angle) };
}

Vec2f normalize(const Vec2f& v) {
	if (float len = length(v))
		return v / len;
	return { 0.f, 0.f };
}

Vec2f abs(const Vec2f& v) {
	return { abs(v.x), abs(v.y) };
}

Vec2f perp(const Vec2f& v) {
	return { -v.y, v.x };
}

float dot(const Vec2f& a, const Vec2f& b) {
	return a.x * b.x + a.y * b.y;
}

float det(const Vec2f& a, const Vec2f& b) {
	return a.x * b.y - a.y * b.x;
}

Vec2f complex_conjugate(const Vec2f& v) {
	return { v.x, -v.y };
}

Vec2f complex_product(const Vec2f& a, const Vec2f& b) {
	return { a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x };
}

Vec2f complex_square(const Vec2f& v) {
	return { v.x * v.x - v.y * v.y, 2.f * v.x * v.y };
}

float angle_unsigned(const Vec2f& a, const Vec2f& b) {
	if (float len2 = length_squared(a) * length_squared(b)) {
		return acos(dot(a, b) / sqrt(len2));
	}
	return 0.f;
}

float angle_signed(const Vec2f& a, const Vec2f& b) {
	return atan2(det(a, b), dot(a, b));
}

bool clockwise(const Vec2f& a, const Vec2f& b) {
	return det(a, b) > 0; // Since y-axis is down, this is the opposite of the usual definition.
}

Vec2f rotate(const Vec2f& v, float angle) {
	float c = cos(angle);
	float s = sin(angle);
	return { v.x * c - v.y * s, v.x * s + v.y * c };
}

Vec2f min(const Vec2f& a, const Vec2f& b) {
	return { std::min(a.x, b.x), std::min(a.y, b.y) };
}

Vec2f max(const Vec2f& a, const Vec2f& b) {
	return { std::max(a.x, b.x), std::max(a.y, b.y) };
}

Vec2f lerp(const Vec2f& a, const Vec2f& b, float t) {
	return a + (b - a) * t;
}

Vec2f lerp_polar(const Vec2f& a, const Vec2f& b, float t) {
	float len = lerp(length(a), length(b), t);
	float angle = lerp_angle(atan2(a.y, a.x), atan2(b.y, b.x), t);
	return unit(angle) * len;
}

Vec2f damp(const Vec2f& a, const Vec2f& b, float damping, float dt) {
	damping = std::clamp(damping, 0.f, 1.f);
	dt = std::max(dt, 0.f);
	if (!damping && !dt) return a;
	return lerp(a, b, 1.f - std::pow(damping, dt));
}

Vec2f clamp(const Vec2f& v, const Vec2f& min, const Vec2f& max) {
	return { std::clamp(v.x, min.x, max.x), std::clamp(v.y, min.y, max.y) };
}

char get_direction(const Vec2f& v) {
	if (v.x >= +abs(v.y)) return 'r';
	if (v.x <= -abs(v.y)) return 'l';
	if (v.y >= +abs(v.x)) return 'd';
	if (v.y <= -abs(v.x)) return 'u';
	return ' '; // This should never happen.
}

bool convex(std::span<const Vec2f> polygon) {
	float first_nonzero_det = 0;
	const size_t vertex_count = polygon.size();
	for (size_t i = 0; i < vertex_count; ++i) {
		size_t i0 = i;
		size_t i1 = (i + 1) % vertex_count;
		size_t i2 = (i + 2) % vertex_count;
		float current_det = det(polygon[i0] - polygon[i1], polygon[i2] - polygon[i1]);
		if (!current_det) continue;
		if (!first_nonzero_det) {
			first_nonzero_det = current_det;
			continue;
		}
		if (current_det * first_nonzero_det < 0) return false;
	}
	return true;
}

std::vector<Vec2f> triangulate(std::span<const Vec2f> polygon) {
	// TODO!!! This is not correct, the implementation is wrong.
	//Ear clipping algorithm: https://www.youtube.com/watch?v=d9tytAQbpXM
	size_t vertex_count = polygon.size();
	assert(vertex_count >= 3);
	if (vertex_count == 3) {
		return { polygon[0], polygon[1], polygon[2] };
	}
	bool is_polygon_clockwise = false;
	{
		size_t i1 = 0; // Will be the rightmost vertex.
		for (size_t i = 1; i < vertex_count; ++i) {
			if (polygon[i].x > polygon[i1].x) {
				i1 = i;
			}
		}
		size_t i0 = (i1 + vertex_count - 1) % vertex_count;
		size_t i2 = (i1 + 1) % vertex_count;
		Vec2f v10 = polygon[i0] - polygon[i1];
		Vec2f v12 = polygon[i2] - polygon[i1];
		is_polygon_clockwise = clockwise(v10, v12);
	}
	std::vector<float> angles(vertex_count);
	for (size_t i = 0; i < vertex_count; ++i) {
		size_t i0 = (i + vertex_count - 1) % vertex_count;
		size_t i1 = i;
		size_t i2 = (i + 1) % vertex_count;
		Vec2f v10 = polygon[i0] - polygon[i1];
		Vec2f v12 = polygon[i2] - polygon[i1];
		float angle = angle_unsigned(v10, v12);
		if (clockwise(v10, v12) != is_polygon_clockwise) {
			angle = M_2PI - angle;
		}
		angles[i1] = angle;
	}
	std::vector<Vec2f> polygon_copy(polygon.begin(), polygon.end());
	std::vector<Vec2f> triangles;
	while (vertex_count > 3) {
		size_t i2 = 0; // Will be the ear tip, i.e. the vertex with the smallest angle.
		for (size_t i = 1; i < vertex_count; ++i) {
			if (angles[i] < angles[i2]) {
				i2 = i;
			}
		}
		size_t i0 = (i2 + vertex_count - 2) % vertex_count;
		size_t i1 = (i2 + vertex_count - 1) % vertex_count;
		size_t i3 = (i2 + 1) % vertex_count;
		size_t i4 = (i2 + 2) % vertex_count;
		triangles.push_back(polygon_copy[i1]);
		triangles.push_back(polygon_copy[i2]);
		triangles.push_back(polygon_copy[i3]);
		{
			Vec2f v10 = polygon_copy[i0] - polygon_copy[i1];
			Vec2f v13 = polygon_copy[i3] - polygon_copy[i1];
			float angle = angle_unsigned(v10, v13);
			if (clockwise(v10, v13) != is_polygon_clockwise) {
				angle = M_2PI - angle;
			}
			angles[i1] = angle;
		}
		{
			Vec2f v31 = polygon_copy[i1] - polygon_copy[i3];
			Vec2f v34 = polygon_copy[i4] - polygon_copy[i3];
			float angle = angle_unsigned(v31, v34);
			if (clockwise(v31, v34) != is_polygon_clockwise) {
				angle = M_2PI - angle;
			}
			angles[i3] = angle;
		}
		polygon_copy.erase(polygon_copy.begin() + i2);
		angles.erase(angles.begin() + i2);
		--vertex_count;
	}
	triangles.push_back(polygon_copy[0]);
	triangles.push_back(polygon_copy[1]);
	triangles.push_back(polygon_copy[2]);
	return triangles;
}
