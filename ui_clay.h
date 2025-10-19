#pragma once

namespace ui {
	struct ImageData {
		Handle<graphics::Texture> texture{};
		Vec2u rect_position{}; // texture rect top left corner *in pixels*
		Vec2u rect_size{}; // texture rect size *in pixels*
	};

	void startup();
	void shutdown();
	void update(float dt);
	void layout();
	void render();
}