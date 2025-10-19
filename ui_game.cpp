#include "stdafx.h"
#include "ui_shared.h"
#include "ui_hud.h"
#include "ui_pause_menu.h"

namespace ui {
namespace game {

	void startup() {
		shared::startup();
		hud::startup();
		hud::show = true;
	}

	void shutdown() {
		// empty
	}

	void update(float dt) {
		hud::update(dt);
	}

	void layout() {
		if (hud::show) {
			hud::layout();
		}
		if (pause_menu::show) {
			pause_menu::layout();
		}
	}

} // namepace game
} // namespace ui