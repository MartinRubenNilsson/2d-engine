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

	// Used to lookup a glyph from a codepoint.
	struct GlyphTableEntry {
		char32_t codepoint = 0;
		int glyph_index = -1;

		auto operator<=>(const GlyphTableEntry&) const = default;
	};

	// Used to lookup a glyph texture rect from a codepoint and a pixel height.
	struct GlyphTextureRectTableEntry {
		char32_t codepoint = 0;
		float pixel_height = 0.f;
		int glyph_texture_rect_index = -1; // index into Font::glyph_texture_rects[]

		auto operator<=>(const GlyphTextureRectTableEntry&) const = default;
	};

	struct Font {
		std::vector<unsigned char> data;
		stbtt_fontinfo info{};

		int ascent = 0; // How much above the baseline the font extends.
		int descent = 0; // How much below the baseline the font extends; typically negative.
		int line_gap = 0; // The spacing between one row's descent and the next row's ascent.
		int whitespace_advance = 0; // How much to horizontally advance the pen for a whitespace.

		std::vector<GlyphTableEntry> glyph_table; // sorted by codepoint
		std::vector<GlyphTextureRectTableEntry> glyph_texture_rect_table; // sorted by codepoint first, pixel_height second
		std::vector<GlyphTextureRect> glyph_texture_rects;

		stbtt_pack_context pack_context{};
		std::vector<unsigned char> atlas_pixels; // size = ATLAS_TEXTURE_SIZE * ATLAS_TEXTURE_SIZE
		Handle<graphics::Texture> atlas_texture;
		bool atlas_texture_needs_updating = true;
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

	template <typename T>
	size_t _lower_bound(const T* array, size_t size, const T& value) {
		size_t first = 0;
		while (size > 0) {
			const size_t step = size / 2;
			const size_t middle = first + step;
			if (array[middle] < value) {
				first = middle + 1;
				size -= step + 1;
			} else {
				size = step;
			}
		}
		return first;
	}

	GlyphId get_glyph(Font& font, char32_t codepoint) {
		// First do a binary search of the table to see if it contains the glyph,
		// otherwise find it the slow way using the stbtt API and then save the result.
		std::vector<GlyphTableEntry>& table = font.glyph_table;
		GlyphTableEntry entry{ codepoint };
		const size_t first = _lower_bound(table.data(), table.size(), entry);
		if (first < table.size() && table[first].codepoint == entry.codepoint) {
			return { table[first].glyph_index };
		}
		entry.glyph_index = stbtt_FindGlyphIndex(&font.info, codepoint);
		table.insert(table.begin() + first, entry);
		return { entry.glyph_index };
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
		// First do a binary search of the texture rect table to see if it already has the rect
		// for this combination of codepoint and pixel height, and if so return it.
		std::vector<GlyphTextureRectTableEntry>& table = font.glyph_texture_rect_table;
		GlyphTextureRectTableEntry entry{ codepoint, pixel_height };
		const size_t first = _lower_bound(table.data(), table.size(), entry);
		if (first < table.size() && // if we found it in the table
			table[first].codepoint == entry.codepoint && 
			table[first].pixel_height == entry.pixel_height) {
			return font.glyph_texture_rects[table[first].glyph_texture_rect_index];
		}
		// We didn't find it in the table, so we need to call the stbtt API to pack the glyph in the
		// texture atlas. Then we record the result in the table so we can look it up in the future.
		entry.glyph_texture_rect_index = (int)font.glyph_texture_rects.size();
		table.insert(table.begin() + first, entry);
		stbtt_packedchar packed_char{};
		stbtt_PackFontRange(&font.pack_context, font.data.data(), 0, pixel_height, codepoint, 1, &packed_char);
		font.atlas_texture_needs_updating = true;
		GlyphTextureRect& rect = font.glyph_texture_rects.emplace_back();
		rect.min.x = packed_char.x0 / (float)ATLAS_TEXTURE_SIZE;
		rect.min.y = packed_char.y0 / (float)ATLAS_TEXTURE_SIZE;
		rect.max.x = packed_char.x1 / (float)ATLAS_TEXTURE_SIZE;
		rect.max.y = packed_char.y1 / (float)ATLAS_TEXTURE_SIZE;
		return rect;
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