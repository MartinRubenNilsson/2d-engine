#include "stdafx.h"
#include "input.h"
#include "window.h"
#include "window_events.h"
#include "console.h"

namespace input {
	constexpr size_t KEY_COUNT = (size_t)Key::Last - (size_t)Key::First + 1;
	std::bitset<KEY_COUNT> _pressed_keys;
	std::bitset<KEY_COUNT> _held_keys;
	std::bitset<KEY_COUNT> _released_keys;

	size_t _to_index(Key key) {
		return (size_t)key - (size_t)Key::First;
	}

	bool pressed(Key key) {
		return _pressed_keys[_to_index(key)];
	}

	bool down(Key key) {
		return _held_keys[_to_index(key)];
	}

	bool released(Key key) {
		return _released_keys[_to_index(key)];
	}

	constexpr size_t MOUSE_BUTTON_COUNT = (size_t)MouseButton::Count;
	std::bitset<MOUSE_BUTTON_COUNT> _pressed_mouse_buttons;
	std::bitset<MOUSE_BUTTON_COUNT> _held_mouse_buttons;
	std::bitset<MOUSE_BUTTON_COUNT> _released_mouse_buttons;

	bool pressed(MouseButton button) {
		return _pressed_mouse_buttons[(size_t)button];
	}

	bool down(MouseButton button) {
		return _held_mouse_buttons[(size_t)button];
	}

	bool released(MouseButton button) {
		return _released_mouse_buttons[(size_t)button];
	}

	Vec2d _mouse_position{}; // relative to the window client rect

	const Vec2d& get_mouse_position() {
		return _mouse_position;
	}

	float get_x_axis() {
		return (float)down(Key::Right) - (float)down(Key::Left);
	}

	float get_y_axis() {
		return (float)down(Key::Down) - (float)down(Key::Up);
	}

	Vec2f get_dir() {
		const float x_axis = get_x_axis();
		const float y_axis = get_y_axis();
		return normalize({ x_axis, y_axis });
	}

	bool _should_capture_input() {
		if (!window::has_focus())
			return false;
		if (console::has_focus())
			return false;
		return true;
	}

	void handle_window_events() {
		_pressed_keys.reset();
		_released_keys.reset();
		_pressed_mouse_buttons.reset();
		_released_mouse_buttons.reset();

		if (!_should_capture_input()) {
			_held_keys.reset();
			_held_mouse_buttons.reset();
			return;
		}

		if (ImGui::GetIO().WantCaptureMouse) {
			_held_mouse_buttons.reset();
			_mouse_position = Vec2d::ZERO;
			return;
		}

		for (const window::Event& ev : window::get_events()) {
			switch (ev.type) {
				case window::EventType::KeyPress: {
					_pressed_keys.set(_to_index(ev.key.code));
				} break;
				case window::EventType::KeyRelease: {
					_released_keys.set(_to_index(ev.key.code));
				} break;
				case window::EventType::MouseButtonPress: {
					_pressed_mouse_buttons.set((size_t)ev.mouse_button.button);
				} break;
				case window::EventType::MouseButtonRelease: {
					_released_mouse_buttons.set((size_t)ev.mouse_button.button);
				} break;
				case window::EventType::MouseMove: {
					_mouse_position.x = ev.mouse_move.x;
					_mouse_position.y = ev.mouse_move.y;
				} break;
			}
		}

		_held_keys |= _pressed_keys;
		_held_keys &= ~_released_keys;
		_held_mouse_buttons |= _pressed_mouse_buttons;
		_held_mouse_buttons &= _released_mouse_buttons;
	}
}