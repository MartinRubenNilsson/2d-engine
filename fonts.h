#pragma once

namespace fonts {
	struct Font;

	struct GlyphId {
		int index = -1;
		auto operator<=>(const GlyphId&) const = default;
	};

	struct GlyphInfo {
		// The horizontal distance to move the pen after drawing this glyph.
		int advance_width = 0;
		// The distance from the glyph origin to the left edge of the glyph.
		int left_side_bearing = 0;
		// The bounding box relative to the glyph's origin.
		int x0 = 0;
		int y0 = 0;
		int x1 = 0;
		int y1 = 0;
		// The texture coordinates (in pixels) of the glyph in the atlas texture.
		int s0 = 0;
		int t0 = 0;
		int s1 = 0;
		int t1 = 0;
	};

	extern const int ATLAS_TEXTURE_SIZE;

	Handle<Font> load_font(const std::string& path);
	Font* get_font(Handle<Font> handle);

	Handle<graphics::Texture> get_atlas_texture(Font& font); // updates the atlas if it is dirty

	// How much a glyph can extend above the baseline.
	int get_ascent(const Font& font);
	// How much a glyph can extend below the baseline. (PITFALL: Negative!)
	int get_descent(const Font& font);
	// The gap between the current line's descent and the next line's ascent. (PITFALL: Positive!)
	int get_line_gap(const Font& font);
	// The gap between the current line's baseline and the next line's baseline.
	// (This is just given by line_spacing = ascent - descent + line_gap.)
	int get_line_spacing(const Font& font);
	// The advance width of the whitespace (" ") character.
	int get_whitespace_width(const Font& font);

	float get_scale_for_pixel_height(const Font& font, float pixel_height);

	GlyphId get_glyph_id(Font& font, char32_t codepoint);
	bool is_glyph_empty(const Font& font, GlyphId glyph);
	GlyphInfo get_glyph_info(Font& font, GlyphId glyph, float pixel_height, char32_t codepoint);
	int get_kerning_advance(Font& font, GlyphId glyph1, GlyphId glyph2);
}
