#pragma once

namespace window {
	bool startup();
	void shutdown();

	void* get_glfw_window(); // Get the GLFW window handle (GLFWwindow).
	void* get_win32_window(); // Get the Win32 window handle (HWND).

	double get_elapsed_time(); // Time since the window was created, in seconds.
	bool should_close();
	void set_should_close(bool should_close);
	bool has_focus();
	void set_visible(bool visible);
	bool visible();
	void set_minimized(bool minimized);
	bool minimized();
	void set_fullscreen(bool fullscreen);
	bool get_fullscreen();
	void set_position(const Vec2i& position);
	Vec2i get_position();
	void set_size(const Vec2i& size);
	Vec2i get_size();
	Vec2i get_framebuffer_size();
	void set_title(const std::string& title);
	void set_icon(unsigned int width, unsigned int height, unsigned char* pixels);
	void set_clipboard_string(const std::string& string); // UTF-8
	std::string_view get_clipboard_string(); // UTF-8
}
