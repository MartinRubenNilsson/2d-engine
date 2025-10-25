#include "stdafx.h"
#include "ui_pause_menu.h"
#include "ui_shared.h"
#include "map.h"

#include "ui_rmlui.h" // TODO: will be removed later

namespace ui {
namespace pause_menu {
	bool show = false;

	void layout() {
		CLAY(CLAY_ID("pause_menu"), shared::menu_with_gray_bg_element) {
			shared::layout_text_button("Resume", bindings::on_click_resume);
			shared::layout_text_button("Restart", [] { show = false; map::reset(); });
			shared::layout_text_button("Settings", bindings::on_click_settings);
			shared::layout_text_button("Main Menu", bindings::on_click_main_menu);
		}
	}
}
}