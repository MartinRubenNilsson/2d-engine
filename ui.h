#pragma once

namespace ui {
	extern bool debug;

	void startup();
	void shutdown();
	void update(float dt);
	void layout();
	void render();

	// Was the left mouse button pressed this frame?
	bool mouse_down();
	// Is the left mouse button currently being held down?
	bool mouse_pressed();
	// Was the left mouse button released this frame?
	bool mouse_up();
	// Is the mouse hovering over the current element?
	bool mouse_over();
	// Did the mouse start hovering the current element this frame?
	bool mouse_enter();
	// Did the mouse stop hovering the current element this frame?
	bool mouse_leave();
}