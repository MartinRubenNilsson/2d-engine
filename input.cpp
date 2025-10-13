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

	bool held(Key key) {
		return _held_keys[_to_index(key)];
	}

	bool released(Key key) {
		return _released_keys[_to_index(key)];
	}

	void update() {
		_pressed_keys.reset();
		_released_keys.reset();

		if (!window::has_focus() || console::has_focus()) {
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