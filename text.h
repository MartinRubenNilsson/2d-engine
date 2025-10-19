#pragma once

namespace text {
	// TODO: horizontal alignment (center each line)

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
		std::string string; // UTF-8 string (even through we're using a regular std::string and not std::u8string).
		Vec2f position{}; // In world units.
		Vec2f shadow_offset{}; // In world units.
		Color color = Color::WHITE;
		Color shadow_color = Color::BLACK;
		float font_size = 8.f; // In world units.
		FontId font{};
		TextAnchor anchor = TextAnchor::Default;
		bool linear_sampling = true; // Otherwise uses nearest sampling.
	};

	Rect2f get_bounding_box(const Text& text);

	void draw_later(const Text& text);
	void sort_all(); // Sorts all texts by draw order.
	void draw_all_now();
}
