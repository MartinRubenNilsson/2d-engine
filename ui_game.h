#pragma once

namespace ui {
namespace game {
	void startup();
	void shutdown();
	void update(float dt);
	void layout();

	bool should_pause_game();
	bool should_blur_background();

	void set_cursor_hovering();
}
}