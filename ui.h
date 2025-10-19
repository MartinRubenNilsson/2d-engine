#pragma once

namespace ui {
	extern bool debug;

	void startup();
	void shutdown();
	void update(float dt);
	void layout();
	void render();
}