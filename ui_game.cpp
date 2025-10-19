#include "stdafx.h"
#include "ui_hud.h"

namespace ui {
namespace game {

	void startup() {
		hud::startup();
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

} // namepace ui
} // namespace game