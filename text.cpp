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

            const float whitespace_width = fonts::get_whitespace_width(*font);
            const float line_spacing = fonts::get_line_spacing(*font);
            const float scale = fonts::get_scale_for_pixel_height(*font, text.pixel_height);

            graphics::temp_vertices.reserve(graphics::temp_vertices.size() + text.string.size());

            Vec2f pos; // Aka "pen" - current position to draw the glyph at.
            fonts::GlyphId prev_glyph{};
            for (char32_t codepoint : text.string) { // codepoint
                if (codepoint == U'\r')
                    continue; // Skip carriage returns to avoid graphical issues

                const fonts::GlyphId glyph = fonts::get_glyph_id(*font, codepoint);

                pos.x += fonts::get_kerning_advance(*font, prev_glyph, glyph);

                switch (codepoint) {
                    case U' ': {
                        pos.x += whitespace_width;
                    } continue;
                    case U'\t': {
                        pos.x += whitespace_width * 4.f; // 1 tab = 4 whitespaces
                    } continue;
                    case U'\n': {
                        pos.x = 0.f;
                        pos.y -= line_spacing;
                    } continue;
                }

                const fonts::GlyphInfo info = fonts::get_glyph_info(*font, glyph, text.pixel_height, codepoint);

                const Vec2f min = pos + Vec2f((float)info.x0, (float)info.y0);
                const Vec2f max = pos + Vec2f((float)info.x1, (float)info.y1);
                // PITFALL: We need to flip the texture v-coordinate (t0 and t1) here
                // because we flip the vertex y-coordinate futher below.
                const Vec2f tex_min = Vec2f((float)info.s0, (float)info.t1) / (float)fonts::ATLAS_TEXTURE_SIZE;
                const Vec2f tex_max = Vec2f((float)info.s1, (float)info.t0) / (float)fonts::ATLAS_TEXTURE_SIZE;

                graphics::temp_vertices.emplace_back(Vec2f(min.x, min.y), colors::WHITE, Vec2f(tex_min.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, min.y), colors::WHITE, Vec2f(tex_max.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(min.x, max.y), colors::WHITE, Vec2f(tex_min.x, tex_max.y));
                graphics::temp_vertices.emplace_back(Vec2f(min.x, max.y), colors::WHITE, Vec2f(tex_min.x, tex_max.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, min.y), colors::WHITE, Vec2f(tex_max.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, max.y), colors::WHITE, Vec2f(tex_max.x, tex_max.y));

                for (unsigned int gv = 0; gv < 6; ++gv) { // gv = glyph vertex
                    const unsigned int v = batch.vertex_offset + batch.vertex_count + gv;
                    graphics::Vertex& vertex = graphics::temp_vertices[v];
                     // The glyphs use a coordinate system with y up, so we must flip.
                    vertex.position.y = -vertex.position.y;
                    vertex.position *= scale;
                    vertex.position *= text.scale;
                    vertex.position += text.position;
                }

                batch.vertex_count += 6;
                pos.x += info.advance_width;
                prev_glyph = glyph;
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