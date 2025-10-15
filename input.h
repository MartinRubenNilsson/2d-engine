#pragma once
#include "window_keys.h"

namespace input {
	using window::Key;

	bool pressed(Key key);
	bool held(Key key);
	bool released(Key key);

	float get_x_axis(); // one of -1, 0, 1
	float get_y_axis(); // one of -1, 0, 1
	Vec2f get_dir(); // zero or unit vector

	void handle_window_events();
}