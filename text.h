#pragma once

namespace text {
	std::u32string to_u32(std::string_view string);

	// TODO: u8string
	// TODO: vertical alignment (baseline, ascent, descent)
	// TODO: horizontal alignment (center each line)

	struct Text {
		Handle<fonts::Font> font{};
		std::u32string string; // String of unicode codepoints; can be created using the U"..." string literal.
		float height = 8.f; // In world units (NOT screen pixels!)
		Vec2f position; // In world units
	};

	// Sorts by draw order.
	bool operator<(const Text& a, const Text& b);

	void draw_later(const Text& text);
	void draw_all_now(std::string_view debug_group_name);
}
