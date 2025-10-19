#include "stdafx.h"
#include "text_shaping.h"
#include "text_fonts.h"
#include "text_unicode.h"

namespace text {
	void shape_text(
        TextShape& shape,
        std::string_view string, // UTF-8 string
        Font& font,
        float font_size,
        float font_texel_density, // Only used if output_texture_rects = true.
        bool output_bounding_boxes, // If true, shape.glyph_bounding_boxes will be populated.
        bool output_texture_rects // If true, shape.glyph_texture_rects will be populated.
	) {
        shape.glyph_count = 0;
        shape.glyph_bounding_boxes.clear();
        shape.glyph_texture_rects.clear();
        shape.bounding_box = Rect2f::EMPTY;

        if (string.empty())
            return;

        // The texel resolution (in number of texels from ascender to descender) the glyphs will have.
        // This will usually be much larger than font_size since there are many pixels on screen per
        // world-space length unit.
        const float font_texel_size = font_size * font_texel_density;

        const int whitespace_advance = get_whitespace_advance(font); // How much to horizontally advance past a whitespace.
        const int line_spacing = get_line_spacing(font); // How much to vertically advance on a newline.

        Vec2f glyph_pos; // Aka "current position" or "pen". This is the origin for where to draw the next glyph.
        GlyphId prev_glyph{};

        // This string view will shrink as the codepoints are being decoded.
        std::u8string_view u8string = { (const char8_t*)string.data(), string.size() };
        while (char32_t codepoint = to_c32(u8string)) { // Decode the next codepoint.
            if (codepoint == U'\r')
                continue; // Ignore carriage returns.

            const GlyphId glyph = get_glyph(font, codepoint);

            glyph_pos.x += get_kerning_advance(font, prev_glyph, glyph);

            switch (codepoint) {
                case U' ': {
                    glyph_pos.x += whitespace_advance;
                } continue;
                case U'\t': {
                    glyph_pos.x += whitespace_advance * 4.f; // 1 tab = 4 whitespaces
                } continue;
                case U'\n': {
                    glyph_pos.x = 0.f;
                    glyph_pos.y += line_spacing; // Move the origin *down* a line.
                } continue;
            }

            // Skip empty glyphs (i.e. whitespaces).
            if (empty(font, glyph)) {
                glyph_pos.x += get_advance(font, glyph);
                prev_glyph = glyph;
            }

            shape.glyph_count++;

            Rect2f box = get_bounding_box(font, glyph);
            // PITFALL: The glyphs use a local coordinate system with positive y up, while our game world uses a
            // coordinate system with positive y down, so we need to flip the sign here.
            box.min.y = -box.min.y;
            box.max.y = -box.max.y;
            // PITFALL: We must also swap the min.y and max.y, since the above sign flip has caused min.y > max.y.
            std::swap(box.min.y, box.max.y);
            // Translate the glyph box to the pen position.
            box.min += glyph_pos;
            box.max += glyph_pos;

            // Expand the text bounding box to include the bounding box of the glyph.
            shape.bounding_box = join(shape.bounding_box, box);

            if (output_bounding_boxes) {
                shape.glyph_bounding_boxes.push_back(box);
            }
            
            if (output_texture_rects) {
                // PITFALL: It's vital to use font_texel_size here and not just font_size! Otherwise the texel density
                // of the glyphs will not match their size on screen and they will appear to have very low resolution.
                const Rect2f rect = get_texture_rect(font, glyph, font_texel_size);
                shape.glyph_texture_rects.push_back(rect);
            }

            glyph_pos.x += get_advance(font, glyph);
            prev_glyph = glyph;
        }

        // How much the glyphs need to be scaled in order to appear font_size units high in world-space.
        const float scale = get_scale_for_font_size(font, font_size);

        // Scale all bounding boxes so they have the correct world-space size.
        shape.bounding_box.min *= scale;
        shape.bounding_box.max *= scale;
        for (Rect2f& box : shape.glyph_bounding_boxes) {
            box.min *= scale;
            box.max *= scale;
        }
	}
}