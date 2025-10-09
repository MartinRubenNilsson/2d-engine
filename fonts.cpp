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

	struct Font {
		std::vector<unsigned char> data;
		stbtt_fontinfo info{};
		int ascent = 0; // The (unscaled) coordinate above the baseline the font extends.
		int descent = 0; // The (unscaled) coordinate below the baseline the font extends; typically negative.
		int line_gap = 0; // The (unscaled) spacing between one row's descent and the next row's ascent.

		std::vector<unsigned char> atlas_pixels; // size = ATLAS_TEXTURE_SIZE * ATLAS_TEXTURE_SIZE
		stbtt_pack_context pack_context{};
		std::unordered_map<char32_t, stbtt_packedchar> packed_chars;
		Handle<graphics::Texture> atlas_texture;
		bool atlas_texture_needs_updating = true;
	};

	Pool<Font> _font_pool;
	std::unordered_map<std::string, Handle<Font>> _font_path_to_handle;

	Handle<Font> load_font(const std::string& path) {
		const std::string normalized_path = filesystem::get_normalized_path(path);
		if (auto it = _font_path_to_handle.find(normalized_path);  it != _font_path_to_handle.end()) {
			return it->second;
		}

		Font font{};
		font.data;
		if (!filesystem::read_binary_file(path, font.data)) {
			console::log_error("Failed to open font file: " + normalized_path);
			return Handle<Font>();
		}

		stbtt_fontinfo info{};
		if (!stbtt_InitFont(&font.info, font.data.data(), 0)) {
			console::log_error("Failed to load font: " + normalized_path);
			return Handle<Font>();
		}

		stbtt_GetFontVMetrics(&font.info, &font.ascent, &font.descent, &font.line_gap);

		font.atlas_pixels.resize(ATLAS_TEXTURE_SIZE * ATLAS_TEXTURE_SIZE);
		stbtt_PackBegin(&font.pack_context, font.atlas_pixels.data(), ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE, 0, 1, nullptr);
		font.atlas_texture = graphics::create_texture({
			.debug_name = normalized_path,
			.width = ATLAS_TEXTURE_SIZE,
			.height = ATLAS_TEXTURE_SIZE,
			.format = graphics::Format::R8_UNORM });

#if 0
		graphics::set_texture_filter(font.atlas_texture, graphics::Filter::Linear);
#endif

		// IMPORTANT: We must move construct the font, otherwise
		// font.data/font.atlas_pixels are reallocated and
		// font.info/font.pack_context are invalidated.
		const Handle<Font> handle = _font_pool.emplace(std::move(font));
		_font_path_to_handle[normalized_path] = handle;

		return handle;
	}

	Font* get_font(Handle<Font> handle) {
		return _font_pool.get(handle);
	}

	Handle<graphics::Texture> get_atlas_texture(Font& font) {
		if (font.atlas_texture_needs_updating) {
			graphics::update_texture(font.atlas_texture, font.atlas_pixels.data());
			font.atlas_texture_needs_updating = false;
		}
		return font.atlas_texture;
	}

	int get_line_spacing(const Font& font) {
		return font.ascent - font.descent + font.line_gap;
	}

	float get_scale_for_pixel_height(const Font& font, float pixel_height) {
		return stbtt_ScaleForPixelHeight(&font.info, pixel_height);
	}

	Glyph get_glyph(Font& font, char32_t codepoint) {
		Glyph glyph{};
		stbtt_GetCodepointHMetrics(&font.info, codepoint, &glyph.advance_width, &glyph.left_side_bearing);
		stbtt_GetCodepointBox(&font.info, codepoint, &glyph.x0, &glyph.y0, &glyph.x1, &glyph.y1);
		auto it = font.packed_chars.find(codepoint);
		if (it == font.packed_chars.end()) {
			const float font_size = 30.f; //Hardcoded for now
			stbtt_packedchar packed_char{};
			stbtt_PackFontRange(&font.pack_context, font.data.data(), 0, font_size, codepoint, 1, &packed_char);
			it = font.packed_chars.emplace(codepoint, packed_char).first;
			font.atlas_texture_needs_updating = true;
		}
		glyph.s0 = it->second.x0;
		glyph.t0 = it->second.y0;
		glyph.s1 = it->second.x1;
		glyph.t1 = it->second.y1;
		return glyph;
	}

	int get_kerning_advance(const Font& font, char32_t codepoint1, char32_t codepoint2) {
		return stbtt_GetCodepointKernAdvance(&font.info, codepoint1, codepoint2);
	}
}