#pragma once
#include <stb_truetype.h>
#define first_glyph_index_in_range first_unicode_codepoint_in_range
#define array_of_glyph_indices array_of_unicode_codepoints
STBTT_DEF int stbtt_PackFontRangesGatherRects_Glyph(stbtt_pack_context* spc, const stbtt_fontinfo* info, stbtt_pack_range* ranges, int num_ranges, stbrp_rect* rects);
STBTT_DEF int stbtt_PackFontRangesRenderIntoRects_Glyph(stbtt_pack_context* spc, const stbtt_fontinfo* info, stbtt_pack_range* ranges, int num_ranges, stbrp_rect* rects);
#define stbtt_PackFontRangesGatherRects stbtt_PackFontRangesGatherRects_Glyph
#define stbtt_PackFontRangesRenderIntoRects stbtt_PackFontRangesRenderIntoRects_Glyph
