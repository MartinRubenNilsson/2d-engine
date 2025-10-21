#pragma once

namespace ui {
	// How to use:
	//   static ImageData data{};
	//   CLAY(CLAY_ID("image"), { .layout = { .image = { .imageData = &data } } })
	//
	struct ImageData {
		Handle<graphics::Texture> texture{};
		Vec2u rect_position{}; // texture rect top left corner *in pixels*
		Vec2u rect_size{}; // texture rect size *in pixels*
	};

	// How to use:
	//  static TextData data{};
	//  CLAY_TEXT(CLAY_STRING("text"), CLAY_TEXT_CONFIG({ .userData = &data }));
	//
	struct TextData {
		Vec2f shadow_offset{};
		Color shadow_color = Color::BLACK;
	};
}