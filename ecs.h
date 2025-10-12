#pragma once

namespace ecs {
	void startup(); // Called on app startup.
	void shutdown(); // Called on app shutdown.

	void setup(MapId map);
	void clear();
	void update(float dt);
	// TODO: handle ui event

	void get_camera_bounds(Vec2f& min, Vec2f& max);
	void draw_sprites_now(const Vec2f& camera_min, const Vec2f& camera_max);


	// TODO: make into bools instead!
	enum DEBUG_DRAW_FLAGS {
		DEBUG_DRAW_PHYSICS = 1 << 1,
		DEBUG_DRAW_AI      = 1 << 2,
	};

	extern int debug_flags;

	extern bool debug_physics;
	extern bool debug_terrain;

	void debug_draw();
}

