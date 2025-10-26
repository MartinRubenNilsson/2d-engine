#pragma once

namespace postprocessing {
	// Calling render() applies the following effects in order:
	// 
	// 1. Shockwaves
	// 2. Lighting
	// 3. Screen transition
	// 4. Gaussian blur

	void startup();
	void update(float dt);
	void render(const Rect2f& view);

	void add_shockwave(const Vec2f& position_ws); // ws = world space
	void set_darkness_intensity(float intensity); // 0 <= intensity <= 1
	void set_darkness_center(const Vec2f& position_ws); // ws = world space
	void set_screen_transition_progress(float progress); // -1 <= progress <= 1
	void set_gaussian_blur_iterations(size_t iterations); // Each Gaussian blur uses a 9x9 kernel
}

