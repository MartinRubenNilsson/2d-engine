#pragma once

namespace ui {
namespace hud {
	extern bool show;
	extern unsigned int max_health;
	extern unsigned int health;
	extern unsigned int rupees;
	extern unsigned int arrows;
	extern unsigned int bombs;

	void startup();
	void update();
	void layout();
}
}
