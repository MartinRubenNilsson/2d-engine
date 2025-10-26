#include "stdafx.h"
#include "ui_pause_menu.h"
#include "ui_settings_menu.h"
#include "ui_main_menu.h"
#include "ui_shared.h"
#include "map.h"
#include "input.h"

namespace ui {
namespace pause_menu {
	bool show = false;

	void _on_select_resume() {
		show = false;
	}

	void _on_select_restart() {
		show = false;
		map::reset();
	}

	void _on_select_settings() {
		show = false;
		settings_menu::show = true;
	}

	void _on_select_main_menu() {
		show = false;
		map::close([] {
			main_menu::show = true;
			});
	}

	void update() {
		if (input::pressed(window::Key::Escape)) {
			_on_select_resume();
		}
	}

	void layout() {
		CLAY(CLAY_ID("pause_menu"), shared::menu_with_gray_bg_element) {
			shared::layout_text_button("Resume", _on_select_resume);
			shared::layout_text_button("Restart", _on_select_restart);
			shared::layout_text_button("Settings", _on_select_settings);
			shared::layout_text_button("Main Menu", _on_select_main_menu);
		}
	}
}
}