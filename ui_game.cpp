#include "stdafx.h"
#include "ui_shared.h"
#include "ui_hud.h"

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
	}

} // namepace game
} // namespace ui