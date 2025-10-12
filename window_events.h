#pragma once
#include "window_keys.h"

namespace window {
	enum class MouseButton {
		Button1,
		Button2,
		Button3,
		Button4,
		Button5,
		Button6,
		Button7,
		Button8,
		Left = Button1,
		Right = Button2,
		Middle = Button3,
	};

	enum MODIFIER_KEY_FLAGS {
		MODIFIER_KEY_SHIFT = (1 << 1), // If this bit is set one or more Shift keys were held down.
		MODIFIER_KEY_CONTROL = (1 << 2), // If this bit is set one or more Control keys were held down.
		MODIFIER_KEY_ALT = (1 << 3), // If this bit is set one or more Alt keys were held down.
		MODIFIER_KEY_SUPER = (1 << 4), // If this bit is set one or more Super keys were held down.
		MODIFIER_KEY_CAPS_LOCK = (1 << 5), // If this bit is set the Caps Lock key is enabled.
		MODIFIER_KEY_NUM_LOCK = (1 << 6), // If this bit is set the Num Lock key is enabled.
	};

	enum class EventType {
		WindowClose, // User attemped to close the window by clicking the close widget or using a key chord like Alt + F4.
		WindowSize, // Window was resized.
		FramebufferSize, // Window framebuffer was resized.
		KeyPress, // A keyboard key was pressed.
		KeyRepeat, // A keyboard key was held down until it started repeating.
		KeyRelease, // A keyboard key was released.
		MouseButtonPress, // A mouse button was pressed.
		MouseButtonRelease, // A mouse button was released.
		MouseMove, // The mouse moved.
	};

	struct SizeEvent {
		int width;
		int height;
	};

	struct KeyEvent {
		Key code;
		int scancode;
		int modifier_key_flags;
	};

	struct MouseButtonEvent {
		MouseButton button;
		int modifier_key_flags;
	};

	struct MouseMoveEvent {
		double x;
		double y;
	};

	struct Event {
		EventType type;
		union {
			SizeEvent size;
			KeyEvent key;
			MouseButtonEvent mouse_button;
			MouseMoveEvent mouse_move;
		};
	};

	std::span<const Event> get_events();

	void clear_events();
	void add_event(const Event& ev);
}