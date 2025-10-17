#pragma once

namespace background
{
	enum class Type
	{
		None,
		MountainDusk,
	};

	void set_type(Type type);
	void update(float dt);
	void draw_sprites_now(const Vec2f& camera_min, const Vec2f& camera_max);
}

