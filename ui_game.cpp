#include "stdafx.h"
#include "ui_shared.h"
#include "ui_main_menu.h"
#include "ui_credits_menu.h"
#include "ui_pause_menu.h"
#include "ui_hud.h"

namespace ui {
namespace game {

	void startup() {
		shared::startup();
		main_menu::startup();
		hud::startup();
		hud::show = true;
	}

	void shutdown() {
		// empty
	}

	void update(float dt) {
		main_menu::update(dt);
		hud::update(dt);
	}

	void layout() {
		if (hud::show) {
			hud::layout();
		}
		if (main_menu::show) {
			main_menu::layout();
		}
		if (credits_menu::show) {
			credits_menu::layout();
		}
		if (pause_menu::show) {
			pause_menu::layout();
		}
	}
}
}