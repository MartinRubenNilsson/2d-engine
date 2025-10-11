#pragma once

namespace text {
	struct Font;

	Handle<Font> load_font(std::string_view path);
	Font* get_font(Handle<Font> handle);

	// How much each glyph's bounding box should be scaled in order for the text height
	// (as measured from ascent to descent) to have the given height in pixels.
	float get_scale_for_pixel_height(const Font& font, float pixel_height);

	// How much a glyph can extend above the baseline.
	int get_ascent(const Font& font);
	// How much a glyph can extend below the baseline. (PITFALL: Negative!)
	int get_descent(const Font& font);
	// The gap between the current line's descent and the next line's ascent. (PITFALL: Positive!)
	int get_line_gap(const Font& font);
	// The gap between the current and the next line's baselines. (This is just ascent - descent + line_gap.)
	int get_line_spacing(const Font& font);
	// How much to advance horizontally past a whitespace (" ") character.
	int get_whitespace_advance(const Font& font);

	struct GlyphId {
		int index = 0;
		auto operator<=>(const GlyphId&) const = default;
	};

	GlyphId get_glyph(Font& font, char32_t codepoint);

	// True if there's nothing to draw for this glyph (e.g. it is a whitespace).
	bool is_empty(const Font& font, GlyphId glyph);
	// The horizontal distance to move the pen after drawing this glyph.
	int get_advance(const Font& font, GlyphId glyph);
	// Extra horizontal difference to move the pen between drawing two glyphs.
	int get_kerning_advance(const Font& font, GlyphId glyph1, GlyphId glyph2);

	struct GlyphBoundingBox {
		Vec2i min; // in the glyph's local coordinate system
		Vec2i max; // in the glyph's local coordinate system
	};

	// Get the bounding box of the glyph in its local coordinate system. You need to scale it
	// by the value returned by get_scale_for_pixel_height() to get correct results.
	GlyphBoundingBox get_bounding_box(const Font& font, GlyphId glyph);

	struct GlyphTextureRect {
		Vec2f min; // in texture coordinates (range [0, 1])
		Vec2f max; // in texture coordinates (range [0, 1])
	};

	// Get the texture rect of the glyph in the font texture atlas. If the glyph isn't already
	// in the atlas it will be written to it. The same glyph can have different texture rects
	// for different pixel heights.
	GlyphTextureRect get_texture_rect(Font& font, char32_t codepoint, float pixel_height);

	bool atlas_texture_needs_updating(const Font& font);
	void update_atlas_texture(Font& font);
	Handle<graphics::Texture> get_atlas_texture(const Font& font);
}
