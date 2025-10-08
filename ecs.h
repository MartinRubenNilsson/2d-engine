#pragma once

namespace ecs {
	void startup(); // Called on app startup.
	void shutdown(); // Called on app shutdown.

	void setup(MapId map);
	void clear();
	void update(float dt);
	void handle_window_event(const window::Event& ev);
	// TODO: handle ui event

	void get_camera_bounds(Vec2f& min, Vec2f& max);
	void draw_sprites(const Vec2f& camera_min, const Vec2f& camera_max);


	// TODO: make into bools instead!
	enum DEBUG_DRAW_FLAGS {
		DEBUG_DRAW_PHYSICS = 1 << 1,
		DEBUG_DRAW_AI      = 1 << 2,
		DEBUG_DRAW_PLAYER  = 1 << 3,
	};

	extern int debug_flags;

	void debug_draw();
}

