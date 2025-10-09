#pragma once

namespace text {
	std::u32string to_u32(std::string_view string);

	struct Text {
		Handle<fonts::Font> font{};
		std::u32string string; // String of unicode codepoints; can be created using the U"..." string literal.
		float pixel_height = 8.f; // In world units...?
		Vec2f position;
		Vec2f scale = { 1.f, 1.f };
	};

	// Sorts by draw order.
	bool operator<(const Text& a, const Text& b);

	void draw_later(const Text& text);
	void draw_all_now(std::string_view debug_group_name);
}
