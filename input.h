#pragma once
#include "window_keys.h"

namespace input {
	using window::Key;

	bool pressed(Key key);
	bool held(Key key);
	bool released(Key key);

	void update();
}