#pragma once

namespace ui {
	extern bool debug;

	void startup();
	void shutdown();
	void update(float dt);
	void layout();
	void render();

	// Is the mouse hovering over the current element?
	bool mouse_over();
	// Did the mouse start hovering the current element this frame?
	bool mouse_enter();
	// Did the mouse stop hovering the current element this frame?
	bool mouse_leave();
}