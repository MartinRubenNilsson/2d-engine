#include "stdafx.h"
#include "ui_game.h"
#include "ui.h"
#include "ui_shared.h"
#include "ui_main_menu.h"
#include "ui_settings_menu.h"
#include "ui_credits_menu.h"
#include "ui_pause_menu.h"
#include "ui_hud.h"
#include "ui_textboxes.h"
#include "input.h"
#include "map.h"
#include "window.h"
#include "window_cursor.h"
#include "images.h"
#include "console.h"

namespace ui {
namespace game {

	window::CursorId _hand_point_cursor{};
	window::CursorId _hand_point_up_cursor{};
	window::CursorId _hand_grab_cursor{};

	void _create_cursors() {
		const std::string IMAGE_PATH = "assets/textures/cursors/cursor32x32.png";
		images::Image image{};
		if (!images::load_image(IMAGE_PATH, image)) {
			console::log_error("Failed to load cursor image: " + IMAGE_PATH);
			return;
		}
		constexpr Vec2u SIZE = { 32, 32 };
		constexpr unsigned int BYTE_SIZE = SIZE.x * SIZE.y * 4;
		if (image.width == SIZE.x && image.height == SIZE.y * 10 && image.data) {
			uint8_t* pixels = (uint8_t*)image.data;
			_hand_point_cursor = window::create_cursor(SIZE, pixels);
			pixels += BYTE_SIZE;
			_hand_point_up_cursor = window::create_cursor(SIZE, pixels);
			pixels += BYTE_SIZE;
			_hand_grab_cursor = window::create_cursor(SIZE, pixels);
			pixels += BYTE_SIZE;
		}
		images::free_image(image);
	}

	void startup() {
		_create_cursors();
		shared::startup();
		main_menu::startup();
		hud::startup();
		textboxes::startup();
	}

	void shutdown() {
		// empty
	}

	void update(float dt) {
		if (main_menu::show) {
			main_menu::update(dt);
		}
		if (settings_menu::show) {
			settings_menu::update();
		}
		if (credits_menu::show) {
			credits_menu::update();
		}
		if (pause_menu::show) {
			pause_menu::update();
		} else {
			if (map::has_current_map() && input::pressed(window::Key::Escape)) {
				pause_menu::show = true;
			}
		}
		hud::show = map::has_current_map();
		if (hud::show) {
			hud::update();
		}
		textboxes::update(dt);
	}

	bool _cursor_hovering = false;

	void layout() {
		_cursor_hovering = false;
		if (hud::show) {
			hud::layout();
		}
		if (main_menu::show) {
			main_menu::layout();
		}
		if (settings_menu::show) {
			settings_menu::layout();
		}
		if (credits_menu::show) {
			credits_menu::layout();
		}
		if (pause_menu::show) {
			pause_menu::layout();
		}
		textboxes::layout();
		if (_cursor_hovering) {
			window::set_cursor(_hand_point_up_cursor);
		} else {
			window::set_cursor(_hand_point_cursor);
		}
	}

	bool should_pause_game() {
		if (main_menu::show) return true;
		if (settings_menu::show) return true;
		if (credits_menu::show) return true;
		if (pause_menu::show) return true;
		if (!textboxes::closed()) return true;
		return false;
	}

	bool should_blur_background() {
		if (settings_menu::show) return true;
		if (credits_menu::show) return true;
		if (pause_menu::show) return true;
		return false;
	}

	void set_cursor_hovering() {
		_cursor_hovering = true;
	}
}
}