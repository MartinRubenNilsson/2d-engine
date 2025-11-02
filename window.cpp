#include "stdafx.h"
#include "window.h"
#include "window_graphics.h"
#include "window_cursor.h"
#include "console.h"
#include "images.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h> // For glfwGetWin32Window()

namespace window {
	GLFWwindow* _window = nullptr;

	void _error_callback(int error, const char* description) {
		console::log_error("GLFW error: " + std::string(description));
	}

	void _set_event_callbacks(); // window_events.cpp

	void _create_standard_cursors(); // window_cursor.cpp

	bool startup() {
		glfwSetErrorCallback(_error_callback);

		if (!glfwInit())
			return false;

#ifdef GRAPHICS_API_OPENGL
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GRAPHICS_API_OPENGL_VERSION_MAJOR);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GRAPHICS_API_OPENGL_VERSION_MINOR);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef _DEBUG_GRAPHICS
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
#else
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hide the window until we're ready to show it.
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		_window = glfwCreateWindow(
			GAME_FRAMEBUFFER_WIDTH,
			GAME_FRAMEBUFFER_HEIGHT,
			WINDOW_TITLE,
			nullptr,
			nullptr);
		if (!_window) return false;

		_set_event_callbacks();
		_create_standard_cursors();
		set_cursor(arrow_cursor);

		return true;
	}

	void _destroy_cursors(); // window_cursor.cpp

	void shutdown() {
		_destroy_cursors();
		glfwDestroyWindow(_window);
		glfwTerminate();
	}

	void* get_glfw_window() {
		return _window;
	}

	void* get_win32_window() {
		return glfwGetWin32Window(_window);
	}

	double get_elapsed_time() {
		return glfwGetTime();
	}

	bool should_close() {
		return glfwWindowShouldClose(_window);
	}

	void set_should_close(bool should_close) {
		glfwSetWindowShouldClose(_window, should_close);
	}

	bool has_focus() {
		return glfwGetWindowAttrib(_window, GLFW_FOCUSED);
	}

	void set_visible(bool visible) {
		if (visible) {
			glfwShowWindow(_window);
		} else {
			glfwHideWindow(_window);
		}
	}

	bool visible() {
		return glfwGetWindowAttrib(_window, GLFW_VISIBLE);
	}

	void set_minimized(bool minimized) {
		if (minimized) {
			glfwIconifyWindow(_window);
		} else {
			glfwRestoreWindow(_window);
		}
	}

	bool minimized() {
		return glfwGetWindowAttrib(_window, GLFW_ICONIFIED);
	}

	void set_fullscreen(bool fullscreen) {
		if (fullscreen == get_fullscreen())
			return;
		static int last_windowed_xpos = 0;
		static int last_windowed_ypos = 0;
		static int last_windowed_width = 0;
		static int last_windowed_height = 0;
		if (!fullscreen) {
			glfwSetWindowMonitor(_window, nullptr,
				last_windowed_xpos,
				last_windowed_ypos,
				last_windowed_width,
				last_windowed_height,
				GLFW_DONT_CARE);
			return;
		}
		GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
		if (!primary_monitor) return;
		const GLFWvidmode* video_mode = glfwGetVideoMode(primary_monitor);
		if (!video_mode) return;
		glfwGetWindowPos(_window, &last_windowed_xpos, &last_windowed_ypos);
		glfwGetWindowSize(_window, &last_windowed_width, &last_windowed_height);
		glfwSetWindowMonitor(_window, primary_monitor,
			0, 0, video_mode->width, video_mode->height, video_mode->refreshRate);
	}

	bool get_fullscreen() {
		return glfwGetWindowMonitor(_window) != nullptr;
	}

	void set_position(const Vec2i& position) {
		glfwSetWindowPos(_window, position.x, position.y);
	}

	Vec2i get_position() {
		Vec2i position{};
		glfwGetWindowPos(_window, &position.x, &position.y);
		return position;
	}

	void set_size(const Vec2i& size) {
		glfwSetWindowSize(_window, size.x, size.y);
	}

	Vec2i get_size() {
		Vec2i size;
		glfwGetWindowSize(_window, &size.x, &size.y);
		return size;
	}

	Vec2i get_framebuffer_size() {
		Vec2i size;
		glfwGetFramebufferSize(_window, &size.x, &size.y);
		return size;
	}

	void set_title(const std::string& title) {
		glfwSetWindowTitle(_window, title.c_str());
	}

	void set_icon(unsigned int width, unsigned int height, unsigned char* pixels) {
		GLFWimage image{};
		image.width = (int)width;
		image.height = (int)height;
		image.pixels = pixels;
		glfwSetWindowIcon(_window, 1, &image);
	}

	void set_clipboard_string(const std::string& string) {
		glfwSetClipboardString(nullptr, string.c_str());
	}

	std::string_view get_clipboard_string() {
		const char* string = glfwGetClipboardString(nullptr);
		return string ? string : std::string_view{};
	}

#ifdef GRAPHICS_API_OPENGL
	void make_opengl_context_current() {
		glfwMakeContextCurrent(_window);
	}

	GLADloadproc get_glad_load_proc() {
		return (GLADloadproc)glfwGetProcAddress;
	}

	void set_swap_chain_sync_interval(int sync_interval) {
		// Calling glfwSwapInverval() repetitively made the app lag,
		// so I've added a check here to ensure we only set it if necessary.
		static int last_sync_interval = INT_MAX;
		if (sync_interval == last_sync_interval) return;
		last_sync_interval = sync_interval;
		glfwSwapInterval(sync_interval);
	}

	void present_swap_chain_back_buffer() {
		glfwSwapBuffers(_window);
	}
#endif

#ifdef GRAPHICS_API_VULKAN
	bool is_vulkan_supported() {
		return glfwVulkanSupported();
	}

	std::span<const char*> get_required_vulkan_instance_extensions() {
		uint32_t count = 0;
		const char** extensions = glfwGetRequiredInstanceExtensions(&count);
		return { extensions, count };
	}
#endif
}
