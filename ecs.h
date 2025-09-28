#pragma once

namespace ecs {
	enum DEBUG_FLAGS {
		DEBUG_PHYSICS  = 1 << 1,
		DEBUG_AI       = 1 << 2,
		DEBUG_PLAYER   = 1 << 3,
	};

	extern int debug_flags;

	void initialize(); // Called on app startup.
	void shutdown(); // Called on app shutdown.

	void setup(); // Called on map load.
	void update(float dt);
	void handle_window_event(const window::Event& ev);

	void get_camera_bounds(Vector2f& min, Vector2f& max);
	void draw_sprites(const Vector2f& camera_min, const Vector2f& camera_max);
	void add_debug_shapes_to_render_queue();
}

