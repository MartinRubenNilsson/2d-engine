#pragma once

namespace text {
	struct Font;

	Handle<Font> load_font(std::string_view path);
	Font* get_font(Handle<Font> handle);

	// How much each glyph's bounding box must be scaled in order for their height
	// (as measured from ascent to descent) to be equal to font_size.
	float get_scale_for_font_size(const Font& font, float font_size);

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
	bool empty(const Font& font, GlyphId glyph);
	// The horizontal distance to move the pen after drawing this glyph.
	int get_advance(const Font& font, GlyphId glyph);
	// Extra horizontal difference to move the pen between drawing two glyphs.
	int get_kerning_advance(const Font& font, GlyphId glyph1, GlyphId glyph2);
	// Get the bounding box of the glyph in its local coordinate system. You need to scale it
	// by the value returned by get_scale_for_font_size() to get correct results.
	Rect2i get_bounding_box(const Font& font, GlyphId glyph);
	// Get the texture rect of the glyph in the font texture atlas. If the glyph isn't already
	// in the atlas it will be written to it. The same glyph will have different texture rects
	// for different font sizes. The coordinates are in UV-space (normalized coordinates).
	Rect2f get_texture_rect(Font& font, GlyphId glyph, float font_size);

	bool atlas_texture_needs_updating(const Font& font);
	void update_atlas_texture(Font& font);
	Handle<graphics::Texture> get_atlas_texture(const Font& font);
}
