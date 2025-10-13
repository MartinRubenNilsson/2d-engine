#pragma once

namespace text {
	// TODO: horizontal alignment (center each line)

	struct Font;

	enum class TextAnchor : uint8_t {
		// The default position of the anchor is at the first row's baseline, where the imaginary "pen" that
		// that draws the glyphs has its starting position. This means that the glyphs on the first line will
		// typically extend *above* Text::position. 
		Default,
		UpperLeft,
		UpperCenter,
		UpperRight,
		MiddleLeft,
		MiddleCenter,
		MiddleRight,
		LowerLeft,
		LowerCenter,
		LowerRight,
	};

	struct Text {
		Handle<Font> font{};
		float font_size = 8.f; // In world units (NOT screen pixels!)
		std::u8string string; // String of Unicode codepoints; can be created using u8"...".
		Vec2f position; // In world units
		TextAnchor anchor = TextAnchor::Default;
		Color color = colors::WHITE;
		bool linear_sampling = true; // otherwise uses nearest sampling
	};

	void draw_later(const Text& text);
	void sort_all(); // Sorts all texts by draw order.
	void draw_all_now(std::string_view debug_group_name);
}
