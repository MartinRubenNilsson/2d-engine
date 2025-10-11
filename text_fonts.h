#pragma once

namespace text {
	struct Font;

	Handle<Font> load_font(const std::string& path);
	Font* get_font(Handle<Font> handle);

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
		int index = -1;
		auto operator<=>(const GlyphId&) const = default;
	};

	struct GlyphBoundingBox {
		Vec2i min; // in the glyph's local (unscaled) coordinate system
		Vec2i max; // in the glyph's local (unscaled) coordinate system
	};

	struct GlyphTextureRect {
		Vec2f min; // in texture coordinates (range [0, 1])
		Vec2f max; // in texture coordinates (range [0, 1])
	};

	GlyphId get_glyph(Font& font, char32_t codepoint);

	// True if there's nothing to draw for this glyph.
	bool is_empty(const Font& font, GlyphId glyph);
	// The horizontal distance to move the pen after drawing this glyph.
	int get_advance(const Font& font, GlyphId glyph);
	// Extra horizontal difference to move the pen between drawing two glyphs.
	int get_kerning_advance(const Font& font, GlyphId glyph1, GlyphId glyph2);
	// Get the bounding box of the glyph in its local (unscaled) coordinate system.
	GlyphBoundingBox get_bounding_box(const Font& font, GlyphId glyph);
	// Get the texture rect of the glyph in the font texture atlas.
	GlyphTextureRect get_texture_rect(Font& font, char32_t codepoint, float pixel_height);

	bool atlas_texture_needs_updating(const Font& font);
	void update_atlas_texture(Font& font);
	Handle<graphics::Texture> get_atlas_texture(const Font& font);
}
