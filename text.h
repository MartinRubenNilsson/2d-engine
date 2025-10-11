#pragma once

namespace text {
	// TODO: vertical alignment (baseline, ascent, descent)
	// TODO: horizontal alignment (center each line)

	struct Font;

	struct Text {
		Handle<Font> font{};
		std::u8string string; // String of Unicode codepoints; can be created using u8"...".
		float height = 8.f; // In world units (NOT screen pixels!)
		Vec2f position; // In world units
		Color color = colors::WHITE;
		bool linear_sampling = true; // otherwise uses nearest sampling
	};

	void draw_later(const Text& text);
	void draw_all_now(std::string_view debug_group_name);
}
