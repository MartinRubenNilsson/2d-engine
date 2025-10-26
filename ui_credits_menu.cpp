#include "stdafx.h"
#include "ui_credits_menu.h"
#include "ui_main_menu.h"
#include "ui_shared.h"
#include "input.h"

namespace ui {
namespace credits_menu {
	bool show = false;

	void _on_select_back() {
		show = false;
		main_menu::show = true;
	}

	void update() {
		if (input::pressed(window::Key::Escape)) {
			_on_select_back();
		}
	}

	void layout() {
		CLAY(CLAY_ID("credits_menu"), shared::menu_with_gray_bg_element) {
			shared::layout_text("Gameplay Programmer - Tim White");
			shared::layout_text("Engine Programmer - Martin Nilsson");
			CLAY(CLAY_ID("credits_menu_padding"), { .layout = { .sizing = { .height = CLAY_SIZING_FIXED(4) }}}) {}
			shared::layout_text_button("Back", _on_select_back);
		}
	}
}
}