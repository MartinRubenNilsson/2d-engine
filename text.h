#pragma once

namespace text {
	// TODO: u8string
	// TODO: vertical alignment (baseline, ascent, descent)
	// TODO: horizontal alignment (center each line)

	struct Text {
		Handle<fonts::Font> font{};
		std::u8string string; // String of Unicode codepoints; can be created u8"...".
		float height = 8.f; // In world units (NOT screen pixels!)
		Vec2f position; // In world units
		Color color = colors::WHITE;
	};

	// Sorts by draw order.
	bool operator<(const Text& a, const Text& b);

	void draw_later(const Text& text);
	void draw_all_now(std::string_view debug_group_name);
}
