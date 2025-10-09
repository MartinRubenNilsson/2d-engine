#include "stdafx.h"
#include "text.h"
#include "fonts.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"

namespace text {
    std::u32string to_u32(std::string_view string) {
        return { string.begin(), string.end() };
    }

    std::vector<Text> _texts;

    bool operator<(const Text& a, const Text& b) {
        return a.font < b.font;
    }

    void draw_later(const Text& text) {
        _texts.push_back(text);
    }

    struct Batch {
        Handle<graphics::Texture> texture{};
        unsigned int vertex_offset = 0;
        unsigned int vertex_count = 0;
    };

    std::vector<Batch> _batches;

    Batch& _get_new_or_current_batch(Handle<graphics::Texture> texture) {
        // Do we need to create a new batch?
        if (_batches.empty() || _batches.back().texture != texture)
            return _batches.emplace_back(texture, (unsigned int)graphics::temp_vertices.size());
        // We can continue the current batch.
        return _batches.back();
    }

    void draw_all_now(std::string_view debug_group_name) {
        if (_texts.empty())
            return;

        std::sort(_texts.begin(), _texts.end());

        graphics::temp_vertices.clear();
        _batches.clear();

        const graphics::ScopedDebugGroup debug_group(debug_group_name);

        for (const Text& text : _texts) {
            fonts::Font* font = fonts::get_font(text.font);
            if (!font)
                continue;

            const Handle<graphics::Texture> texture = fonts::get_atlas_texture(*font);
            if (texture == Handle<graphics::Texture>())
                continue;

            Batch& batch = _get_new_or_current_batch(texture);

            float whitespace_width = fonts::get_whitespace_width(*font);
            const float letter_spacing = whitespace_width * (text.letter_spacing_factor - 1.f);
            whitespace_width += letter_spacing;
            const float line_spacing = fonts::get_line_spacing(*font) * text.line_spacing_factor;
            const float scale_for_pixel_height = fonts::get_scale_for_pixel_height(*font, text.pixel_height);

            graphics::temp_vertices.reserve(graphics::temp_vertices.size() + text.string.size());

            Vec2f glyph_pos; // Current glyph position in unscaled coordinates
            char32_t prev_c = 0; // previous codepoint
            for (char32_t c : text.string) { // codepoint
                if (c == U'\r')
                    continue; // Skip carriage returns to avoid graphical issues

                glyph_pos.x += fonts::get_kerning_advance(*font, prev_c, c);

                switch (c) {
                    case U' ': {
                        glyph_pos.x += whitespace_width;
                    } continue; // Don't need to create a quad for whitespaces
                    case U'\t': {
                        glyph_pos.x += whitespace_width * 4.f; // 1 tab = 4 whitespaces
                    } continue; // Don't need to create a quad for whitespaces
                    case U'\n': {
                        glyph_pos.x = 0.f;
                        glyph_pos.y -= line_spacing; // TODO: is this correct?
                    } continue; // Don't need to create a quad for whitespaces
                }

                const fonts::Glyph glyph = fonts::get_glyph(*font, c);

                const Vec2f pos0 = glyph_pos + Vec2f((float)glyph.x0, (float)glyph.y0);
                const Vec2f pos1 = glyph_pos + Vec2f((float)glyph.x1, (float)glyph.y1);
                Vec2f tex0 = Vec2f((float)glyph.s0, (float)glyph.t0) / (float)fonts::ATLAS_TEXTURE_SIZE;
                Vec2f tex1 = Vec2f((float)glyph.s1, (float)glyph.t1) / (float)fonts::ATLAS_TEXTURE_SIZE;
                std::swap(tex0.y, tex1.y); // Flip y-axis TODO but dont we flip it below?

                graphics::temp_vertices.emplace_back(Vec2f(pos0.x, pos0.y), colors::WHITE, Vec2f(tex0.x, tex0.y));
                graphics::temp_vertices.emplace_back(Vec2f(pos1.x, pos0.y), colors::WHITE, Vec2f(tex1.x, tex0.y));
                graphics::temp_vertices.emplace_back(Vec2f(pos0.x, pos1.y), colors::WHITE, Vec2f(tex0.x, tex1.y));
                graphics::temp_vertices.emplace_back(Vec2f(pos0.x, pos1.y), colors::WHITE, Vec2f(tex0.x, tex1.y));
                graphics::temp_vertices.emplace_back(Vec2f(pos1.x, pos0.y), colors::WHITE, Vec2f(tex1.x, tex0.y));
                graphics::temp_vertices.emplace_back(Vec2f(pos1.x, pos1.y), colors::WHITE, Vec2f(tex1.x, tex1.y));

                for (unsigned int gv = 0; gv < 6; ++gv) { // gv = glyph vertex
                    const unsigned int v = batch.vertex_offset + batch.vertex_count + gv;
                    graphics::Vertex& vertex = graphics::temp_vertices[v];
                    // Flip y-axis TODO but dont we flip it above?
                    vertex.position.y = -vertex.position.y;
                    // TODO: scaling is probably wrong??
                    // TODO: no, we have hardcoded the font size!!!
                    vertex.position *= scale_for_pixel_height;
                    vertex.position *= text.scale;
                    vertex.position += text.position;
                }

                batch.vertex_count += 6;
                glyph_pos.x += glyph.advance_width + letter_spacing;
                prev_c = c;
            }
        }

        graphics::set_primitives(graphics::Primitives::TriangleList);
        graphics::bind_vertex_shader(graphics::sprite_vert);
        graphics::bind_fragment_shader(graphics::text_frag);

        //TODO: hide this logic in wrapper functions
        const unsigned int vertices_byte_size = (unsigned int)graphics::temp_vertices.size() * sizeof(graphics::Vertex);
        if (vertices_byte_size <= graphics::get_buffer_size(graphics::dynamic_vertex_buffer)) {
            graphics::update_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices.data(), vertices_byte_size);
        } else {
            graphics::recreate_buffer(graphics::dynamic_vertex_buffer, vertices_byte_size, graphics::temp_vertices.data());
        }

        graphics::bind_vertex_buffer(0, graphics::dynamic_vertex_buffer, sizeof(graphics::Vertex));

        //TODO: different samplers?

        for (const Batch& batch : _batches) {
            graphics::bind_texture(0, batch.texture);
            graphics::draw(batch.vertex_count, batch.vertex_offset);
        }

        graphics::temp_vertices.clear();
        _texts.clear();
        _batches.clear();
    }
}