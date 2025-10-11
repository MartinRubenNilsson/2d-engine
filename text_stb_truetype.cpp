// The functions in this file are alternative version of the packing functions in stb_truetype.h
// that use glyph indices instead of char32_t codepoints. This is better because it means they don't
// have to look up the glyph index from the code point every time.

#include "stdafx.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "text_stb_truetype.h"

// rects array must be big enough to accommodate all characters in the given ranges
STBTT_DEF int stbtt_PackFontRangesGatherRects_Glyph(stbtt_pack_context* spc, const stbtt_fontinfo* info, stbtt_pack_range* ranges, int num_ranges, stbrp_rect* rects) {
    int i, j, k;
    int missing_glyph_added = 0;

    k = 0;
    for (i = 0; i < num_ranges; ++i) {
        float fh = ranges[i].font_size;
        float scale = fh > 0 ? stbtt_ScaleForPixelHeight(info, fh) : stbtt_ScaleForMappingEmToPixels(info, -fh);
        ranges[i].h_oversample = (unsigned char)spc->h_oversample;
        ranges[i].v_oversample = (unsigned char)spc->v_oversample;
        for (j = 0; j < ranges[i].num_chars; ++j) {
            int x0, y0, x1, y1;
            // NOTE: This line was changed and the one after that was removed.
            int glyph = ranges[i].array_of_glyph_indices == NULL ? ranges[i].first_glyph_index_in_range + j : ranges[i].array_of_glyph_indices[j];
            if (glyph == 0 && (spc->skip_missing || missing_glyph_added)) {
                rects[k].w = rects[k].h = 0;
            } else {
                stbtt_GetGlyphBitmapBoxSubpixel(info, glyph,
                    scale * spc->h_oversample,
                    scale * spc->v_oversample,
                    0, 0,
                    &x0, &y0, &x1, &y1);
                rects[k].w = (stbrp_coord)(x1 - x0 + spc->padding + spc->h_oversample - 1);
                rects[k].h = (stbrp_coord)(y1 - y0 + spc->padding + spc->v_oversample - 1);
                if (glyph == 0)
                    missing_glyph_added = 1;
            }
            ++k;
        }
    }

    return k;
}

// rects array must be big enough to accommodate all characters in the given ranges
STBTT_DEF int stbtt_PackFontRangesRenderIntoRects_Glyph(stbtt_pack_context* spc, const stbtt_fontinfo* info, stbtt_pack_range* ranges, int num_ranges, stbrp_rect* rects) {
    int i, j, k, missing_glyph = -1, return_value = 1;

    // save current values
    int old_h_over = spc->h_oversample;
    int old_v_over = spc->v_oversample;

    k = 0;
    for (i = 0; i < num_ranges; ++i) {
        float fh = ranges[i].font_size;
        float scale = fh > 0 ? stbtt_ScaleForPixelHeight(info, fh) : stbtt_ScaleForMappingEmToPixels(info, -fh);
        float recip_h, recip_v, sub_x, sub_y;
        spc->h_oversample = ranges[i].h_oversample;
        spc->v_oversample = ranges[i].v_oversample;
        recip_h = 1.0f / spc->h_oversample;
        recip_v = 1.0f / spc->v_oversample;
        sub_x = stbtt__oversample_shift(spc->h_oversample);
        sub_y = stbtt__oversample_shift(spc->v_oversample);
        for (j = 0; j < ranges[i].num_chars; ++j) {
            stbrp_rect* r = &rects[k];
            if (r->was_packed && r->w != 0 && r->h != 0) {
                stbtt_packedchar* bc = &ranges[i].chardata_for_range[j];
                int advance, lsb, x0, y0, x1, y1;
                // NOTE: This line was changed and the one after that was removed.
                int glyph = ranges[i].array_of_glyph_indices == NULL ? ranges[i].first_glyph_index_in_range + j : ranges[i].array_of_glyph_indices[j];
                stbrp_coord pad = (stbrp_coord)spc->padding;

                // pad on left and top
                r->x += pad;
                r->y += pad;
                r->w -= pad;
                r->h -= pad;
                stbtt_GetGlyphHMetrics(info, glyph, &advance, &lsb);
                stbtt_GetGlyphBitmapBox(info, glyph,
                    scale * spc->h_oversample,
                    scale * spc->v_oversample,
                    &x0, &y0, &x1, &y1);
                stbtt_MakeGlyphBitmapSubpixel(info,
                    spc->pixels + r->x + r->y * spc->stride_in_bytes,
                    r->w - spc->h_oversample + 1,
                    r->h - spc->v_oversample + 1,
                    spc->stride_in_bytes,
                    scale * spc->h_oversample,
                    scale * spc->v_oversample,
                    0, 0,
                    glyph);

                if (spc->h_oversample > 1)
                    stbtt__h_prefilter(spc->pixels + r->x + r->y * spc->stride_in_bytes,
                        r->w, r->h, spc->stride_in_bytes,
                        spc->h_oversample);

                if (spc->v_oversample > 1)
                    stbtt__v_prefilter(spc->pixels + r->x + r->y * spc->stride_in_bytes,
                        r->w, r->h, spc->stride_in_bytes,
                        spc->v_oversample);

                bc->x0 = (stbtt_int16)r->x;
                bc->y0 = (stbtt_int16)r->y;
                bc->x1 = (stbtt_int16)(r->x + r->w);
                bc->y1 = (stbtt_int16)(r->y + r->h);
                bc->xadvance = scale * advance;
                bc->xoff = (float)x0 * recip_h + sub_x;
                bc->yoff = (float)y0 * recip_v + sub_y;
                bc->xoff2 = (x0 + r->w) * recip_h + sub_x;
                bc->yoff2 = (y0 + r->h) * recip_v + sub_y;

                if (glyph == 0)
                    missing_glyph = j;
            } else if (spc->skip_missing) {
                return_value = 0;
            } else if (r->was_packed && r->w == 0 && r->h == 0 && missing_glyph >= 0) {
                ranges[i].chardata_for_range[j] = ranges[i].chardata_for_range[missing_glyph];
            } else {
                return_value = 0; // if any fail, report failure
            }

            ++k;
        }
    }

    // restore original values
    spc->h_oversample = old_h_over;
    spc->v_oversample = old_v_over;

    return return_value;
}