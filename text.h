#pragma once

namespace text {
	// TODO: horizontal alignment (center each line)

	struct Font;

	enum class TextOrigin : uint8_t {
		Default, // Origin is at the first row's baseline (so the first row will extend above the origin).
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
		std::u8string string; // String of Unicode codepoints; can be created using u8"...".
		float letter_height = 8.f; // In world units (NOT screen pixels!)
		Vec2f position; // In world units
		TextOrigin origin = TextOrigin::Default;
		Color color = colors::WHITE;
		bool linear_sampling = true; // otherwise uses nearest sampling
	};

	void draw_later(const Text& text);
	void sort_all(); // Sorts all texts by draw order.
	void draw_all_now(std::string_view debug_group_name);
}
