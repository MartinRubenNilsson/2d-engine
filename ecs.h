#pragma once

namespace ecs {
	void startup(); // Called on app startup.
	void shutdown(); // Called on app shutdown.

	void setup(MapId map);
	void clear();
	void update(float dt);
	void debug();

	void get_camera_bounds(Vec2f& min, Vec2f& max);
	void draw_sprites_now(const Vec2f& camera_min, const Vec2f& camera_max);
}

