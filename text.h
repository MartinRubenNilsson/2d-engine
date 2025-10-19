#pragma once

namespace text {
	// TODO: horizontal alignment (center each line)

	enum class TextAnchor : uint8_t {
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
		TextAnchor anchor = TextAnchor::UpperLeft;
		bool linear_sampling = true; // Otherwise uses nearest sampling.
	};

	Rect2f get_bounding_box(const Text& text);

	void draw_later(const Text& text);
	void sort_all(); // Sorts all texts by draw order.
	void draw_all_now();
}
