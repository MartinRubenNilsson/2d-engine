#pragma once
#include "window_keys.h"
#include "window_mouse_buttons.h"

namespace input {
	using window::Key;
	using window::MouseButton;

	bool pressed(Key key);
	bool down(Key key);
	bool released(Key key);

	bool pressed(MouseButton button);
	bool down(MouseButton button);
	bool released(MouseButton button);

	const Vec2d& get_mouse_position();
	const Vec2d& get_mouse_scroll_delta();

	float get_x_axis(); // one of -1, 0, 1
	float get_y_axis(); // one of -1, 0, 1
	Vec2f get_dir(); // zero or unit vector

	void handle_window_events();
}