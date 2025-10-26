#pragma once

namespace postprocessing {
	void startup();
	void update(float dt);
	void render_pre_ui(const Rect2f& view);
	void render_post_ui(const Rect2f& view);

	void add_shockwave(const Vec2f& position_ws); // ws = world space
	void set_darkness_intensity(float intensity); // 0 <= intensity <= 1
	void set_darkness_center(const Vec2f& position_ws); // ws = world space
	void set_screen_transition_progress(float progress); // -1 <= progress <= 1
	void set_gaussian_blur_iterations(size_t iterations); // Each Gaussian blur uses a 9x9 kernel
}

