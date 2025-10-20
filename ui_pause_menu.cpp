#include "stdafx.h"
#include "ui_pause_menu.h"
#include "ui_shared.h"

#include "ui_rmlui.h" // TODO: will be removed later

namespace ui {
namespace pause_menu {
	bool show = false;

	void layout() {
		CLAY(CLAY_ID("pause_menu"), { .layout = {
			.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
			.childGap = 6,
			.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
			.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.backgroundColor = Color(64, 64, 64, 76),
			.floating = { .attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			shared::layout_menu_button("Resume", bindings::on_click_resume);
			shared::layout_menu_button("Restart", bindings::on_click_restart);
			shared::layout_menu_button("Settings", bindings::on_click_settings);
			shared::layout_menu_button("Main Menu", bindings::on_click_main_menu);
		}
	}
}
}