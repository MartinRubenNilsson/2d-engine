#pragma once

namespace ecs {
	void startup(); // Called on app startup.
	void shutdown(); // Called on app shutdown.

	void setup(MapId map); // Called on map load.
	void clear(); // Called on map unload.
	void update(float dt);
	void debug_draw(const Rect2f& view);

	Rect2f get_camera_view();
	void draw_sprites_now(const Rect2f& view);
}

