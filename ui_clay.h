#pragma once

namespace ui {
	bool startup();
	void shutdown();

	void update(float dt);
	void layout();
	void render(const graphics::Viewport& viewport);
}