#pragma once

namespace ui {
	struct ImageData {
		Handle<graphics::Texture> texture{};
		Vec2u rect_position{}; // texture rect top left corner *in pixels*
		Vec2u rect_size{}; // texture rect size *in pixels*
	};

	// Optional extra text layout info that can be passed to .userData.
	struct TextData {
		Vec2f shadow_offset{};
		Color shadow_color = Color::BLACK;
	};
}