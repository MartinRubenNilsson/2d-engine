#include "stdafx.h"
#include "ui_credits_menu.h"
#include "ui_shared.h"

#include "ui_rmlui.h" // TODO: will be removed later

namespace ui {
	namespace credits_menu {
		bool show = false;

		void layout() {
			CLAY(CLAY_ID("credits_menu"), shared::menu_with_gray_bg_element) {
				shared::layout_text("Gameplay Programmer - Tim White");
				shared::layout_text("Engine Programmer - Martin Nilsson");
				CLAY(CLAY_ID("credits_menu_padding"), { .layout = { .sizing = { .height = CLAY_SIZING_FIXED(4) }}}) {}
				shared::layout_menu_button("Back", bindings::on_click_back);
			}
		}
	}
}