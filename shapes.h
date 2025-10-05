#pragma once
#ifdef _DEBUG

namespace shapes {
	const unsigned int MAX_POLYGON_VERTICES = 8;

	void update_lifetimes(float dt);
	void draw_all(std::string_view debug_group_name, const Vec2f& camera_min, const Vec2f& camera_max);

	void add_point(const Vec2f& point, const Color& color = colors::WHITE, float lifetime = 0.f);
	void add_line(const Vec2f& p1, const Vec2f& p2, const Color& color = colors::WHITE, float lifetime = 0.f);
	void add_box(const Vec2f& min, const Vec2f& max, const Color& color = colors::WHITE, float lifetime = 0.f);
	void add_polygon(const Vec2f* points, unsigned int count, const Color& color = colors::WHITE, float lifetime = 0.f);
	void add_circle(const Vec2f& center, float radius, const Color& color = colors::WHITE, float lifetime = 0.f);
}

#endif // _DEBUG