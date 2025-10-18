#include "stdafx.h"
#include "text_fonts.h"
#include "text_stb_truetype.h"
#include "console.h"
#include "graphics.h"
#include "files.h"
#include <stb_rect_pack.h>

namespace text {
	// Used to lookup a glyph from a codepoint.
	struct GlyphTableEntry {
		char32_t codepoint = 0;
		int glyph_index = 0;

		auto operator<=>(const GlyphTableEntry&) const = default;
	};

	// Used to lookup the kerning between two glyphs.
	struct GlyphKerningTableEntry {
		int glyph1_index = 0;
		int glyph2_index = 0;
		int advance = 0;

		auto operator<=>(const GlyphKerningTableEntry&) const = default;
	};

	// Used to lookup a glyph texture rect from a glyph index and a font size.
	struct GlyphTexRectTableEntry {
		int glyph_index = 0;
		float font_size = 0.f;
		int glyph_tex_rect_index = -1; // index into Font::glyph_tex_rects[]

		auto operator<=>(const GlyphTexRectTableEntry&) const = default;
	};

	struct Font {
		std::string path;
		std::vector<unsigned char> data;

		stbtt_fontinfo info{};
		stbtt_pack_context pack_context{};

		int ascent = 0; // How much above the baseline the font extends.
		int descent = 0; // How much below the baseline the font extends; typically negative.
		int line_gap = 0; // The spacing between one row's descent and the next row's ascent.
		int whitespace_advance = 0; // How much to horizontally advance the pen for a whitespace.

		std::vector<GlyphTableEntry> glyph_table; // sorted by codepoint
		std::vector<GlyphKerningTableEntry> glyph_kerning_table; // sorted by glyph1_index first, glyph2_index second
		std::vector<GlyphTexRectTableEntry> glyph_tex_rect_table; // sorted by glyph_index first, then font_size
		std::vector<Rect2f> glyph_tex_rects;

		std::vector<unsigned char> atlas_pixels; // size = ATLAS_TEXTURE_SIZE * ATLAS_TEXTURE_SIZE
		Handle<graphics::Texture> atlas_texture;
		bool atlas_texture_needs_updating = false;
	};

	std::vector<Font> _fonts;

	FontId::operator bool() const {
		return id < _fonts.size();
	}

	void startup_fonts() {
		// TODO: load default font
	}

	void shutdown_fonts() {
		for (Font& font : _fonts) {
			stbtt_PackEnd(&font.pack_context);
		}
		_fonts.clear();
	}

	constexpr int ATLAS_TEXTURE_SIZE = 1024;

	FontId load_font(std::string_view path) {
		std::string normalized_path = files::get_normalized_path(path);
		for (uint16_t id = 0; id < _fonts.size(); ++id) {
			if (_fonts[id].path == normalized_path) {
				return { id };
			}
		}

		Font font{};
		font.path = std::move(normalized_path);

		if (!files::read_binary_file(font.path, font.data)) {
			console::log_error("Failed to open font file: " + font.path);
			return {};
		}

		stbtt_fontinfo info{};
		if (!stbtt_InitFont(&font.info, font.data.data(), 0)) {
			console::log_error("Failed to initialize font: " + font.path);
			return {};
		}

		stbtt_GetFontVMetrics(&font.info, &font.ascent, &font.descent, &font.line_gap);
		// Precompute the whitespace advance because we look it up often.
		font.whitespace_advance = get_advance(font, get_glyph(font, U' '));

		// Setup kerning table.
		{
			const int table_length = stbtt_GetKerningTableLength(&font.info);
			font.glyph_kerning_table.resize(table_length);
			stbtt_GetKerningTable(&font.info, (stbtt_kerningentry*)(font.glyph_kerning_table.data()), table_length);
		}

		// Setup texture atlas.
		font.atlas_pixels.resize(ATLAS_TEXTURE_SIZE * ATLAS_TEXTURE_SIZE);
		font.atlas_texture = graphics::create_texture({
			.debug_name = font.path,
			.width = ATLAS_TEXTURE_SIZE,
			.height = ATLAS_TEXTURE_SIZE,
			.format = graphics::Format::R8_UNORM });

		// Setup pack.
		stbtt_PackBegin(&font.pack_context, font.atlas_pixels.data(),
			ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE, 0, 1, nullptr);

		const uint16_t id = (uint16_t)_fonts.size();
		// IMPORTANT: We must move construct the font, otherwise
		// font.data/font.atlas_pixels are reallocated and
		// font.info/font.pack_context are invalidated.
		_fonts.emplace_back(std::move(font));
		return { id };
	}

	Font& get_font(FontId font) {
		return _fonts[font.id];
	}

	float get_scale_for_font_size(const Font& font, float font_size) {
		return stbtt_ScaleForPixelHeight(&font.info, font_size);
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

	// I made this because std::lower_bound() was rather slow in debug.
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
		// otherwise find it the (potentially) slower way using the stbtt API.
		auto& table = font.glyph_table;
		GlyphTableEntry entry{ codepoint };
		const size_t first = _lower_bound(table.data(), table.size(), entry);
		if (first < table.size() && table[first].codepoint == entry.codepoint) {
			return { table[first].glyph_index };
		}
		entry.glyph_index = stbtt_FindGlyphIndex(&font.info, codepoint);
		table.insert(table.begin() + first, entry); // Save the result for later.
		return { entry.glyph_index };
	}

	bool empty(const Font& font, GlyphId glyph) {
		return stbtt_IsGlyphEmpty(&font.info, glyph.index);
	}

	int get_advance(const Font& font, GlyphId glyph) {
		int advance_width = 0;
		int left_side_bearing = 0;
		stbtt_GetGlyphHMetrics(&font.info, glyph.index, &advance_width, &left_side_bearing);
		return advance_width;
	}

	int get_kerning_advance(const Font& font, GlyphId glyph1, GlyphId glyph2) {
		const auto& table = font.glyph_kerning_table;
		GlyphKerningTableEntry entry{ glyph1.index, glyph2.index };
		const size_t first = _lower_bound(table.data(), table.size(), entry);
		if (first >= table.size())
			return 0;
		if (table[first].glyph1_index != entry.glyph1_index)
			return 0;
		if (table[first].glyph2_index != entry.glyph2_index)
			return 0;
		return table[first].advance;
	}

	Rect2i get_bounding_box(const Font& font, GlyphId glyph) {
		Rect2i box{};
		stbtt_GetGlyphBox(&font.info, glyph.index, &box.min.x, &box.min.y, &box.max.x, &box.max.y);
		return box;
	}

	// Returns the new texture rect (in UV-space). You need to call update_atlas_texture()
	// after calling this in order for the texture on the GPU to recieve the changes.
	Rect2f _pack_and_rasterize_glyph_to_atlas(Font& font, int glyph_index, float font_size) {
		stbtt_packedchar packed_char{};
		stbtt_pack_range pack_range{};
		pack_range.first_glyph_index_in_range = glyph_index;
		pack_range.chardata_for_range = &packed_char;
		pack_range.num_chars = 1;
		pack_range.font_size = font_size;
		stbrp_rect rect{}; // texture coordinates in texels
		stbtt_PackFontRangesGatherRects(&font.pack_context, &font.info, &pack_range, 1, &rect);
		stbtt_PackFontRangesPackRects(&font.pack_context, &rect, 1);
		stbtt_PackFontRangesRenderIntoRects(&font.pack_context, &font.info, &pack_range, 1, &rect); // may fail!
		font.atlas_texture_needs_updating = true;
		Rect2f tex_rect{}; // texture coordinates in UV-space
		tex_rect.min.x = packed_char.x0 / (float)ATLAS_TEXTURE_SIZE;
		tex_rect.min.y = packed_char.y0 / (float)ATLAS_TEXTURE_SIZE;
		tex_rect.max.x = packed_char.x1 / (float)ATLAS_TEXTURE_SIZE;
		tex_rect.max.y = packed_char.y1 / (float)ATLAS_TEXTURE_SIZE;
		return tex_rect;
	}

	Rect2f get_texture_rect(Font& font, GlyphId glyph, float font_size) {
		// First do a binary search of the texture rect table to see if it already has the rect
		// for this combination of codepoint and pixel height, and if so return it.
		auto& table = font.glyph_tex_rect_table;
		GlyphTexRectTableEntry entry{ glyph.index, font_size };
		const size_t first = _lower_bound(table.data(), table.size(), entry);
		if (first < table.size() &&
			table[first].glyph_index == entry.glyph_index &&
			table[first].font_size == entry.font_size
		) { // if we found it in the table
			const int texture_rect_index = table[first].glyph_tex_rect_index;
			return font.glyph_tex_rects[texture_rect_index];
		}
		// We didn't find it in the table, so we need to call the stbtt API to pack the glyph in the
		// texture atlas. Then we record the result in the table so we can look it up in the future.
		entry.glyph_tex_rect_index = (int)font.glyph_tex_rects.size();
		table.insert(table.begin() + first, entry);
		Rect2f tex_rect = _pack_and_rasterize_glyph_to_atlas(font, glyph.index, font_size);
		font.glyph_tex_rects.push_back(tex_rect);
		return tex_rect;
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