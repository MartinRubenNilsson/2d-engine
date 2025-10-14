#pragma once

namespace ui {
	bool startup_clay();
	void shutdown_clay();

	void set_clay_pointer_state(float x, float y, bool is_down);

	void update_clay(float dt);
	void layout_clay();
	void render_clay();
}