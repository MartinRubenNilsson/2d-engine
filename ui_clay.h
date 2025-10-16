#pragma once

namespace ui {
	struct Image {
		Handle<graphics::Texture> texture{};
		Vec2u tex_rect_pos{}; // texture rect top left corner position in pixels
		Vec2u tex_rect_size{}; // texture rect size in pixels
	};

	uint16_t to_id(Handle<text::Font> font);

	void startup();
	void shutdown();
	void update(float dt);
	void layout();
	void render(const graphics::Viewport& viewport);
}