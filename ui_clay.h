#pragma once

namespace ui {
	struct Image {
		Handle<graphics::Texture> texture{};
		Rect2f tex_rect{}; // uv coordinates
	};

	void startup();
	void shutdown();
	void update(float dt);
	void layout();
	void render(const graphics::Viewport& viewport);
}