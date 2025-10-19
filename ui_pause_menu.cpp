#include "stdafx.h"
#include "ui_pause_menu.h"
#include "ui_shared.h"

namespace ui {
namespace pause_menu {
	bool show = true; // for testing

	void layout() {
		CLAY(CLAY_ID("pause_menu"), { .layout = {
			.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
			.childGap = 6,
			.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
			.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.floating = { .attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			shared::layout_menu_button("Resume", []() { show = false; });
			shared::layout_menu_button("Restart", []() { show = false; });
			shared::layout_menu_button("Settings", []() { show = false; });
			shared::layout_menu_button("Main Menu", []() { show = false; });
		}
	}
}
}