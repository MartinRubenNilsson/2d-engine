#pragma once

namespace fonts {
	struct Font;

	struct Glyph {
		// The horizontal distance (in unscaled coordinates) to move the pen after drawing this glyph.
		int advance_width = 0;
		// The distance (in unscaled coordinates) from the glyph origin to the left edge of the glyph.
		int left_side_bearing = 0;
		// The bounding box (in unscaled coordinates) relative to the glyph's origin.
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
	int get_line_spacing(const Font& font); // in unscaled coordinates
	int get_whitespace_width(const Font& font); // in unscaled coordinates
	float get_scale_for_pixel_height(const Font& font, float pixel_height);
	int get_glyph_index(Font& font, char32_t codepoint);
	Glyph get_glyph(Font& font, char32_t codepoint);
	int get_kerning_advance(Font& font, char32_t codepoint1, char32_t codepoint2); // in unscaled coordinates
}
