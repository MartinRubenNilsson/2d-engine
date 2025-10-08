#pragma once

namespace text {
	std::u32string to_u32(std::string_view string);

	struct Text {
		Handle<fonts::Font> font{};
		std::u32string string; // String of unicode codepoints; can be created using the U"..." string literal.
		float pixel_height = 30.f;
		float letter_spacing_factor = 1.f;
		float line_spacing_factor = 1.f;
		Vec2f position;
		Vec2f scale = { 1.f, 1.f };
	};

	//TODO
	//draw_text_later(const Test& test);
	//sort_all();
	//draw_all();

	void draw(const Text& text);
}
