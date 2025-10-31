#pragma once

namespace ecs {
	void startup();
	void shutdown();
	void setup(MapId map);
	void clear();
	void update(float dt);

	Rect2f get_camera_view();
	void draw_sprites_now(const Rect2f& view);
	void debug_draw(const Rect2f& view);
}

