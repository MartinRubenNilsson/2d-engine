#include "stdafx.h"
#include "ui_settings_menu.h"
#include "ui_main_menu.h"
#include "ui_pause_menu.h"
#include "ui_shared.h"
#include "map.h"
#include "input.h"

namespace ui {
namespace settings_menu {
	bool show = false;

	void _on_select_back() {
		show = false;
		if (map::is_open()) {
			pause_menu::show = true;
		} else {
			main_menu::show = true;
		}
	}

	void update() {
		if (input::pressed(window::Key::Escape)) {
			_on_select_back();
		}
	}

	void layout() {
		CLAY(CLAY_ID("settings_menu"), shared::menu_with_gray_bg_element) {
			shared::layout_text_button("Back", _on_select_back);
		}
	}
}
}