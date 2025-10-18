#pragma once

namespace engine {
	extern bool should_show_fps_counter;

	void update_fps_counter(float dt);
	void show_fps_counter_imgui();
}