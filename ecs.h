#pragma once

namespace ecs {
	void startup(); // Called on app startup.
	void shutdown(); // Called on app shutdown.

	void setup(MapId map); // Called on map load.
	void clear(); // Called on map unload.
	void update(float dt);
	void debug_draw();

	void get_camera_bounds(Vec2f& min, Vec2f& max);
	void draw_sprites_now(const Vec2f& camera_min, const Vec2f& camera_max);
}

