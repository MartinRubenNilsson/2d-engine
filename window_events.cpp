#include "stdafx.h"
#include "window_events.h"
#include <GLFW/glfw3.h>

namespace window {
	std::vector<Event> _events_front_buffer;
	std::vector<Event> _events_back_buffer;

	std::span<const Event> get_events() {
		return _events_front_buffer;
	}

	// Adds the event to the *back buffer*.
	void _add_event(const Event& ev) {
		_events_back_buffer.push_back(ev);
	}

	void _window_close_callback(GLFWwindow* window) {
		// GLFW sets the close flag before invoking this callback,
		// so we need to unset it so the window doesn't immediately close.
		glfwSetWindowShouldClose(window, GLFW_FALSE);
		_add_event({ .type = EventType::WindowClose });
	}

	void _window_size_callback(GLFWwindow* window, int width, int height) {
		Event ev{};
		ev.type = EventType::WindowSize;
		ev.size.width = width;
		ev.size.height = height;
		_add_event(ev);
	}

	void _framebuffer_size_callback(GLFWwindow* window, int width, int height) {
		Event ev{};
		ev.type = EventType::FramebufferSize;
		ev.size.width = width;
		ev.size.height = height;
		_add_event(ev);
	}

	int _translate_modifier_key_flags(int glfw_modifier_key_flags) {
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
		ev.key.modifier_key_flags = _translate_modifier_key_flags(mods);
		_add_event(ev);
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
		ev.mouse_button.modifier_key_flags = _translate_modifier_key_flags(mods);
		_add_event(ev);
	}

	void _cursor_pos_callback(GLFWwindow* window, double x, double y) {
		Event ev{};
		ev.type = EventType::MouseMove;
		ev.mouse_move.x = x;
		ev.mouse_move.y = y;
		_add_event(ev);
	}

	void _mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
		Event ev{};
		ev.type = EventType::MouseScroll;
		ev.mouse_scroll.delta_x = xoffset;
		ev.mouse_scroll.delta_y = xoffset;
		_add_event(ev);
	}

	extern GLFWwindow* _window;

	void _set_event_callbacks() {
		glfwSetWindowCloseCallback(_window, _window_close_callback);
		glfwSetKeyCallback(_window, _key_callback);
		glfwSetWindowSizeCallback(_window, _window_size_callback);
		glfwSetFramebufferSizeCallback(_window, _framebuffer_size_callback);
		glfwSetMouseButtonCallback(_window, _mouse_button_callback);
		glfwSetCursorPosCallback(_window, _cursor_pos_callback);
		glfwSetScrollCallback(_window, _mouse_scroll_callback);
	}

	void update_events() {
		static bool first_time = true;
		if (first_time) { // HACK: Spoof resize events to ensure that other systems are aware of the window/framebuffer size.
			int width, height;
			glfwGetWindowSize(_window, &width, &height);
			_window_size_callback(_window, width, height);
			glfwGetFramebufferSize(_window, &width, &height);
			_framebuffer_size_callback(_window, width, height);
			first_time = false;
		}
		glfwPollEvents(); // Will add events to the *back buffer*.
		std::swap(_events_front_buffer, _events_back_buffer);
		_events_back_buffer.clear();

#if 1
		for (const window::Event& ev : get_events()) {
			if (ev.type == window::EventType::WindowClose) {
				glfwSetWindowShouldClose(_window, GLFW_TRUE);
			}
		}
#endif
	}
}
