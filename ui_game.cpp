#include "stdafx.h"
#include "ui_shared.h"
#include "ui_main_menu.h"
#include "ui_settings_menu.h"
#include "ui_credits_menu.h"
#include "ui_pause_menu.h"
#include "ui_hud.h"
#include "ui_textboxes.h"
#include "ui_game.h"
#include "input.h"
#include "map.h"

namespace ui {
namespace game {

	void startup() {
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
			if (map::is_open() && input::pressed(window::Key::Escape)) {
				pause_menu::show = true;
			}
		}
		hud::show = map::is_open();
		if (hud::show) {
			hud::update();
		}
		textboxes::update(dt);
	}

	void layout() {
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
}
}