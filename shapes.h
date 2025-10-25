#pragma once

namespace shapes {
	const unsigned int MAX_POLYGON_VERTICES = 8;

	void draw_point_later(const Vec2f& point, const Color& color = Color::WHITE, float lifetime = 0.f);
	void draw_line_later(const Vec2f& p1, const Vec2f& p2, const Color& color = Color::WHITE, float lifetime = 0.f);
	void draw_box_later(const Vec2f& min, const Vec2f& max, const Color& color = Color::WHITE, float lifetime = 0.f);
	void draw_polygon_later(const Vec2f* points, unsigned int count, const Color& color = Color::WHITE, float lifetime = 0.f);
	void draw_circle_later(const Vec2f& center, float radius, const Color& color = Color::WHITE, float lifetime = 0.f);

	void update_lifetimes(float dt);
	void draw_all_now();
}