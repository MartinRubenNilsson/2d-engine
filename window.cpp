#include "stdafx.h"
#include "window.h"
#include "window_events.h"
#include "window_graphics.h"
#include "console.h"
#include "images.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h> // For glfwGetWin32Window()

namespace window {
	GLFWwindow* _window = nullptr;
	GLFWcursor* _cursors[(int)CursorShape::Count] = { nullptr };

	void _error_callback(int error, const char* description) {
		__debugbreak();
		console::log_error("GLFW error: " + std::string(description));
	}

	void _window_close_callback(GLFWwindow* window) {
		// GLFW sets the close flag before invoking this callback,
		// so we need to unset it so the window doesn't immediately close.
		glfwSetWindowShouldClose(window, GLFW_FALSE);
		add_event({ .type = EventType::WindowClose });
	}

	void _window_size_callback(GLFWwindow* window, int width, int height) {
		Event ev{};
		ev.type = EventType::WindowSize;
		ev.size.width = width;
		ev.size.height = height;
		add_event(ev);
	}

	void _framebuffer_size_callback(GLFWwindow* window, int width, int height) {
		Event ev{};
		ev.type = EventType::FramebufferSize;
		ev.size.width = width;
		ev.size.height = height;
		add_event(ev);
	}

	int _translate_modifier_key_flags_from_glfw(int glfw_modifier_key_flags) {
		int modifier_key_flags = 0;
		if (glfw_modifier_key_flags & GLFW_MOD_SHIFT)     modifier_key_flags |= MODIFIER_KEY_SHIFT;
		if (glfw_modifier_key_flags & GLFW_MOD_CONTROL)   modifier_key_flags |= MODIFIER_KEY_CONTROL;
		if (glfw_modifier_key_flags & GLFW_MOD_ALT)       modifier_key_flags |= MODIFIER_KEY_ALT;
		if (glfw_modifier_key_flags & GLFW_MOD_SUPER)     modifier_key_flags |= MODIFIER_KEY_SUPER;
		if (glfw_modifier_key_flags & GLFW_MOD_CAPS_LOCK) modifier_key_flags |= MODIFIER_KEY_CAPS_LOCK;
		if (glfw_modifier_key_flags & GLFW_MOD_NUM_LOCK)  modifier_key_flags |= MODIFIER_KEY_NUM_LOCK;
		return modifier_key_flags;
	}

	void _key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		Event ev{};
		if (action == GLFW_PRESS) {
			ev.type = EventType::KeyPress;
		} else if (action == GLFW_RELEASE) {
			ev.type = EventType::KeyRelease;
		} else if (action == GLFW_REPEAT) {
			ev.type = EventType::KeyRepeat;
		} else {
			return;
		}
		ev.key.code = (Key)key;
		ev.key.scancode = scancode;
		ev.key.modifier_key_flags = _translate_modifier_key_flags_from_glfw(mods);
		add_event(ev);
	}

	void _mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
		Event ev{};
		if (action == GLFW_PRESS) {
			ev.type = EventType::MouseButtonPress;
		} else if (action == GLFW_RELEASE) {
			ev.type = EventType::MouseButtonRelease;
		} else {
			return;
		}
		ev.mouse_button.button = (MouseButton)button;
		ev.mouse_button.modifier_key_flags = _translate_modifier_key_flags_from_glfw(mods);
		add_event(ev);
	}

	void _cursor_pos_callback(GLFWwindow* window, double x, double y) {
		Event ev{};
		ev.type = EventType::MouseMove;
		ev.mouse_move.x = x;
		ev.mouse_move.y = y;
		add_event(ev);
	}

	bool startup() {
		glfwSetErrorCallback(_error_callback);
		if (!glfwInit()) return false;

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

		glfwSetWindowCloseCallback(_window, _window_close_callback);
		glfwSetKeyCallback(_window, _key_callback);
		glfwSetWindowSizeCallback(_window, _window_size_callback);
		glfwSetFramebufferSizeCallback(_window, _framebuffer_size_callback);
		glfwSetMouseButtonCallback(_window, _mouse_button_callback);
		glfwSetCursorPosCallback(_window, _cursor_pos_callback);

		// CREATE STANDARD CURSORS

		_cursors[(int)CursorShape::Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		_cursors[(int)CursorShape::IBeam] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
		_cursors[(int)CursorShape::Crosshair] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
		_cursors[(int)CursorShape::Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
		_cursors[(int)CursorShape::HResize] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
		_cursors[(int)CursorShape::VResize] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);

		// LOAD AND CREATE CUSTOM CURSORS

		images::Image cursor_image{};
		if (!images::load_image("assets/textures/cursors/cursor32x32.png", cursor_image)) {
			console::log_error("Failed to load cursor image");
		} else {
			constexpr int CURSOR_SIZE = 32;
			constexpr int CURSOR_SIZE_BYTES = CURSOR_SIZE * CURSOR_SIZE * 4;
			if (cursor_image.width == CURSOR_SIZE && cursor_image.height == CURSOR_SIZE * 10 && cursor_image.data) {
				GLFWimage image{};
				image.height = CURSOR_SIZE;
				image.width = CURSOR_SIZE;
				image.pixels = (unsigned char*)cursor_image.data;
				_cursors[(int)CursorShape::HandPoint] = glfwCreateCursor(&image, 0, 0);
				image.pixels += CURSOR_SIZE_BYTES;
				_cursors[(int)CursorShape::HandPointUp] = glfwCreateCursor(&image, 0, 0);
				image.pixels += CURSOR_SIZE_BYTES;
				_cursors[(int)CursorShape::HandGrab] = glfwCreateCursor(&image, 0, 0);
				image.pixels += CURSOR_SIZE_BYTES;
				_cursors[(int)CursorShape::Quill] = glfwCreateCursor(&image, 0, 0);
			}
			images::free_image(cursor_image);
		}

		// FINAL SETUP

		set_cursor_shape(CursorShape::HandPoint);
		set_icon_from_file("assets/window/swordsman.png");

		return true;
	}

	void shutdown() {
		for (GLFWcursor* cursor : _cursors) {
			glfwDestroyCursor(cursor);
		}
		memset(_cursors, 0, sizeof(_cursors));
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

	void update_events() {
		clear_events();
		static bool first_time = true;
		if (first_time) { // HACK: Spoof resize events to ensure that other systems are aware of the window/framebuffer size.
			int width, height;
			glfwGetWindowSize(_window, &width, &height);
			_window_size_callback(_window, width, height);
			glfwGetFramebufferSize(_window, &width, &height);
			_framebuffer_size_callback(_window, width, height);
			first_time = false;
		}
		glfwPollEvents();
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

	bool get_visible() {
		return glfwGetWindowAttrib(_window, GLFW_VISIBLE);
	}

	void set_minimized(bool minimized) {
		if (minimized) {
			glfwIconifyWindow(_window);
		} else {
			glfwRestoreWindow(_window);
		}
	}

	bool get_minimized() {
		return glfwGetWindowAttrib(_window, GLFW_ICONIFIED);
	}

	void set_fullscreen(bool fullscreen) {
		if (fullscreen == get_fullscreen()) return;
		static int last_windowed_xpos = 0;
		static int last_windowed_ypos = 0;
		static int last_windowed_width = 0;
		static int last_windowed_height = 0;
		if (fullscreen) {
			GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
			if (!primary_monitor) return;
			const GLFWvidmode* video_mode = glfwGetVideoMode(primary_monitor);
			if (!video_mode) return;
			glfwGetWindowPos(_window, &last_windowed_xpos, &last_windowed_ypos);
			glfwGetWindowSize(_window, &last_windowed_width, &last_windowed_height);
			glfwSetWindowMonitor(_window, primary_monitor,
				0, 0, video_mode->width, video_mode->height, video_mode->refreshRate);
		} else {
			glfwSetWindowMonitor(_window, nullptr,
				last_windowed_xpos, last_windowed_ypos,
				last_windowed_width, last_windowed_height, GLFW_DONT_CARE);
		}
	}

	bool get_fullscreen() {
		return glfwGetWindowMonitor(_window) != nullptr;
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

	void set_icon_from_memory(int width, int height, unsigned char* pixels) {
		GLFWimage image{};
		image.width = width;
		image.height = height;
		image.pixels = pixels;
		glfwSetWindowIcon(_window, 1, &image);
	}

	void set_icon_from_file(const std::string& path) {
		images::Image image{};
		if (!images::load_image(path, image)) {
			console::log_error("Failed to load icon: " + path);
			return;
		}
		set_icon_from_memory(image.width, image.height, (unsigned char*)image.data);
		images::free_image(image);
	}

	void set_cursor_visible(bool visible) {
		glfwSetInputMode(_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
	}

	bool get_cursor_visible() {
		return glfwGetInputMode(_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
	}

	void set_cursor_pos(const Vec2d& pos) {
		glfwSetCursorPos(_window, pos.x, pos.y);
	}

	Vec2d get_cursor_pos() {
		Vec2d pos;
		glfwGetCursorPos(_window, &pos.x, &pos.y);
		return pos;
	}

	void set_cursor_shape(CursorShape shape) {
		glfwSetCursor(_window, _cursors[(int)shape]);
	}

	void set_clipboard_string(const std::u8string& string) {
		glfwSetClipboardString(nullptr, (const char*)string.c_str());
	}

	std::u8string_view get_clipboard_string() {
		const char* string = glfwGetClipboardString(nullptr);
		return string ? (const char8_t*)string : u8"";
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

	bool pressed(Key key) {
		return glfwGetKey(_window, (int)key) == GLFW_PRESS;
	}
}
