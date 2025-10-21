#include "stdafx.h"
#include "input.h"
#include "window.h"
#include "window_events.h"
#include "console.h"
#include "ui_rmlui.h"

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

	bool held(Key key) {
		return _held_keys[_to_index(key)];
	}

	bool released(Key key) {
		return _released_keys[_to_index(key)];
	}

	float get_x_axis() {
		return (float)held(Key::Right) - (float)held(Key::Left);
	}

	float get_y_axis() {
		return (float)held(Key::Down) - (float)held(Key::Up);
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
		if (ui::is_menu_or_visible()) // TODO: this should be "ui::has_focus()" or something!!!
			return false;
		return true;
	}

	void handle_window_events() {
		_pressed_keys.reset();
		_released_keys.reset();

		if (!_should_capture_input()) {
			_held_keys.reset();
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
			}
		}

		_held_keys |= _pressed_keys;
		_held_keys &= ~_released_keys;
	}
}