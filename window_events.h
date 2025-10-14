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
		MouseScroll, // The scroll wheel was scrolled OR the touchpad was swiped.
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

	struct MouseScrollEvent {
		double delta_x;
		double delta_y;
	};

	struct Event {
		EventType type;
		union {
			SizeEvent size;
			KeyEvent key;
			MouseButtonEvent mouse_button;
			MouseMoveEvent mouse_move;
			MouseScrollEvent mouse_scroll;
		};
	};

	// PITFALL: New events may be (unintentionally?) created in the middle of a frame by calling certain functions,
	// e.g. calling window::set_size() will create a new SizeEvent. To ensure that each system recieves this event
	// one and only one time, we double buffer the events. Newly created events are added to a "back buffer" while
	// get_events() returns a "front buffer". update_events() swaps these and clears the back buffer.
	std::span<const Event> get_events();

	// Call as early as possible each frame.
	void update_events();
}