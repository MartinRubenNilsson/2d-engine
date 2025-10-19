#pragma once

namespace text {
	struct Font;

	struct TextShape {
		size_t glyph_count = 0; // Number of *nonempty* glyphs (i.e. not including whitespaces).
		Rect2f bounding_box; // Bounding box for the whole text. This is always computed.
		std::vector<Rect2f> glyph_bounding_boxes; // either empty *or* size = glyph_count
		std::vector<Rect2f> glyph_texture_rects; // either empty *or* size = glyph_count
	};

	void shape_text(
		TextShape& shape,
		std::string_view string, // UTF-8 string
		Font& font,
		float font_size,
		float font_texel_density, // Only used if output_texture_rects = true.
		bool output_bounding_boxes, // If true, shape.glyph_bounding_boxes will be populated.
		bool output_texture_rects // If true, shape.glyph_texture_rects will be populated.
	);
}