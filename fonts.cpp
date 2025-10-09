#include "stdafx.h"
#include "fonts.h"
#include "pool.h"
#include "console.h"
#include "graphics.h"
#include "filesystem.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace fonts {
	const int ATLAS_TEXTURE_SIZE = 1024;

	struct CodepointGlyphPair {
		char32_t codepoint = 0;
		int glyph_index = -1;
	};

	bool _compare_codepoints(const CodepointGlyphPair& a, const CodepointGlyphPair& b) {
		return a.codepoint < b.codepoint;
	}

	struct Font {
		std::vector<unsigned char> data;
		stbtt_fontinfo info{};
		int ascent = 0; // The (unscaled) coordinate above the baseline the font extends.
		int descent = 0; // The (unscaled) coordinate below the baseline the font extends; typically negative.
		int line_gap = 0; // The (unscaled) spacing between one row's descent and the next row's ascent.
		int whitespace_advance = 0;

		std::vector<CodepointGlyphPair> codepoint_glyph_pairs; // sorted by codepoint
		std::unordered_map<int, GlyphTextureRect> packed_chars;

		std::vector<unsigned char> atlas_pixels; // size = ATLAS_TEXTURE_SIZE * ATLAS_TEXTURE_SIZE
		Handle<graphics::Texture> atlas_texture;
		bool atlas_texture_needs_updating = true;
		stbtt_pack_context pack_context{};
	};

	Pool<Font> _font_pool;
	std::unordered_map<std::string, Handle<Font>> _font_cache; // path to handle

	Handle<Font> load_font(const std::string& path) {
		const std::string normalized_path = filesystem::get_normalized_path(path);
		if (auto it = _font_cache.find(normalized_path);  it != _font_cache.end()) {
			return it->second;
		}

		Font font{};
		font.data;
		if (!filesystem::read_binary_file(path, font.data)) {
			console::log_error("Failed to open font file: " + normalized_path);
			return {};
		}

		stbtt_fontinfo info{};
		if (!stbtt_InitFont(&font.info, font.data.data(), 0)) {
			console::log_error("Failed to load font: " + normalized_path);
			return {};
		}

		stbtt_GetFontVMetrics(&font.info, &font.ascent, &font.descent, &font.line_gap);

		font.atlas_pixels.resize(ATLAS_TEXTURE_SIZE * ATLAS_TEXTURE_SIZE);
		stbtt_PackBegin(&font.pack_context, font.atlas_pixels.data(), ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE, 0, 1, nullptr);
		font.atlas_texture = graphics::create_texture({
			.debug_name = normalized_path,
			.width = ATLAS_TEXTURE_SIZE,
			.height = ATLAS_TEXTURE_SIZE,
			.format = graphics::Format::R8_UNORM });

		// Precompute because we need it often.
		font.whitespace_advance = get_advance(font, get_glyph(font, U' '));
#if 0
		graphics::set_texture_filter(font.atlas_texture, graphics::Filter::Linear);
#endif

		// IMPORTANT: We must move construct the font, otherwise
		// font.data/font.atlas_pixels are reallocated and
		// font.info/font.pack_context are invalidated.
		const Handle<Font> handle = _font_pool.emplace(std::move(font));
		_font_cache[normalized_path] = handle;

		return handle;
	}

	Font* get_font(Handle<Font> handle) {
		return _font_pool.get(handle);
	}

	float get_scale_for_pixel_height(const Font& font, float pixel_height) {
		return stbtt_ScaleForPixelHeight(&font.info, pixel_height);
	}

	int get_ascent(const Font& font) {
		return font.ascent;
	}

	int get_descent(const Font& font) {
		return font.descent;
	}

	int get_line_gap(const Font& font) {
		return font.line_gap;
	}

	int get_line_spacing(const Font& font) {
		return font.ascent - font.descent + font.line_gap;
	}

	int get_whitespace_advance(const Font& font) {
		return font.whitespace_advance;
	}

	GlyphId get_glyph(Font& font, char32_t codepoint) {
		// First do a binary search of the lookup table to see if it contains the glyph,
		// otherwise find it the slow way using the stbtt API and then save the result.
		std::vector<CodepointGlyphPair>& pairs = font.codepoint_glyph_pairs;
		size_t first = 0;
		size_t count = pairs.size();
		while (count > 0) {
			size_t curr = first;
			size_t step = count / 2;
			curr += step;
			if (pairs[curr].codepoint < codepoint) {
				first = ++curr;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		if (first < pairs.size() && pairs[first].codepoint == codepoint) {
			return { pairs[first].glyph_index };
		}
		const int glyph_index = stbtt_FindGlyphIndex(&font.info, codepoint);
		pairs.emplace(pairs.begin() + first, codepoint, glyph_index);
		return { glyph_index };
	}

	bool is_empty(const Font& font, GlyphId glyph) {
		return stbtt_IsGlyphEmpty(&font.info, glyph.index);
	}

	int get_advance(const Font& font, GlyphId glyph) {
		int advance_width = 0;
		int left_side_bearing_dummy = 0;
		stbtt_GetGlyphHMetrics(&font.info, glyph.index, &advance_width, &left_side_bearing_dummy);
		return advance_width;
	}

	int get_kerning_advance(const Font& font, GlyphId glyph1, GlyphId glyph2) {
		return stbtt_GetGlyphKernAdvance(&font.info, glyph1.index, glyph2.index);
	}

	GlyphBoundingBox get_bounding_box(const Font& font, GlyphId glyph) {
		GlyphBoundingBox box{};
		stbtt_GetGlyphBox(&font.info, glyph.index, &box.min.x, &box.min.y, &box.max.x, &box.max.y);
		return box;
	}

	GlyphTextureRect get_texture_rect(Font& font, char32_t codepoint, float pixel_height) {
		auto it = font.packed_chars.find(codepoint);
		if (it == font.packed_chars.end()) {
			stbtt_packedchar packed_char{};
			stbtt_PackFontRange(&font.pack_context, font.data.data(), 0, 30.f, codepoint, 1, &packed_char);
			GlyphTextureRect rect{};
			rect.min.x = packed_char.x0 / (float)ATLAS_TEXTURE_SIZE;
			rect.min.y = packed_char.y0 / (float)ATLAS_TEXTURE_SIZE;
			rect.max.x = packed_char.x1 / (float)ATLAS_TEXTURE_SIZE;
			rect.max.y = packed_char.y1 / (float)ATLAS_TEXTURE_SIZE;
			it = font.packed_chars.emplace(codepoint, rect).first;
			font.atlas_texture_needs_updating = true;
		}
		return it->second;
	}

	bool atlas_texture_needs_updating(const Font& font) {
		return font.atlas_texture_needs_updating;
	}

	void update_atlas_texture(Font& font) {
		if (!font.atlas_texture_needs_updating) return;
		graphics::update_texture(font.atlas_texture, font.atlas_pixels.data());
		font.atlas_texture_needs_updating = false;
	}

	Handle<graphics::Texture> get_atlas_texture(const Font& font) {
		return font.atlas_texture;
	}
}