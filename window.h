#pragma once

namespace window {
	bool startup();
	void shutdown();

	void* get_glfw_window(); // Get the GLFW window handle (GLFWwindow).
	void* get_win32_window(); // Get the Win32 window handle (HWND).

	double get_elapsed_time(); // Time since the window was created, in seconds.
	bool should_close();
	void set_should_close(bool should_close);
	// Processes all pending events and populates the event queue. Call at the start of each frame.
	void update_events();
	bool has_focus();
	void set_visible(bool visible);
	bool get_visible();
	void set_minimized(bool minimized);
	bool get_minimized();
	void set_fullscreen(bool fullscreen);
	bool get_fullscreen();
	void set_size(const Vec2i& size);
	Vec2i get_size();
	Vec2i get_framebuffer_size();
	void set_title(const std::string& title);
	void set_icon_from_memory(int width, int height, unsigned char* pixels);
	void set_icon_from_file(const std::string& path);
	void set_cursor_visible(bool visible);
	bool get_cursor_visible();
	void set_cursor_pos(const Vec2d& pos);
	Vec2d get_cursor_pos();

	enum class CursorShape {
		// STANDARD CURSORS

		Arrow, // The regular arrow cursor shape.
		IBeam, // The text input I-beam cursor shape.
		Crosshair, // The crosshair shape.
		Hand, // The hand shape.
		HResize, // The horizontal resize arrow shape.
		VResize, // The vertical resize arrow shape.

		// CUSTOM CURSORS

		HandPoint,
		HandPointUp,
		HandGrab,
		Quill,

		Count, // Keep last -- the total number of cursor shapes.
	};

	void set_cursor_shape(CursorShape shape);
	void set_clipboard_string(const std::u8string& string);
	std::u8string_view get_clipboard_string();
}
