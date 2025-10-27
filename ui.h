#pragma once

namespace ui {
	extern bool debug_layout;
	extern bool debug_bounding_boxes;

	void startup();
	void shutdown();
	void update(float dt);
	void begin_layout();
	void end_layout();
	void render();

	Clay_String to_clay(std::string_view string);

	// Was the left mouse button pressed this frame?
	bool mouse_down();
	// Is the left mouse button currently being held down?
	bool mouse_pressed();
	// Was the left mouse button released this frame?
	bool mouse_up();

	// Is the mouse hovering over any element?
	bool mouse_over_any();
	// Is the mouse hovering over the current element?
	bool mouse_over();
	// Did the mouse start hovering the current element this frame?
	bool mouse_enter();
	// Did the mouse stop hovering the current element this frame?
	bool mouse_leave();
}