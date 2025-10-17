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
	void update(float dt);
	void layout();
}

	bool get_hud_visible();
	void set_hud_visible(bool visible);
}
