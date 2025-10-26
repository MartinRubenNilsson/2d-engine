#include "stdafx.h"
#include "ui_settings_menu.h"
#include "ui_shared.h"
#include "ui_rmlui.h"

namespace ui {
namespace settings_menu {
	bool show = false;

	void layout() {
		CLAY(CLAY_ID("settings_menu"), shared::menu_with_gray_bg_element) {
			shared::layout_text_button("Back", bindings::on_click_back);
		}
	}
}
}