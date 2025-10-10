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

        const graphics::ScopedDebugGroup debug_group(debug_group_name);

        std::sort(_texts.begin(), _texts.end());

        graphics::temp_vertices.clear();
        _batches.clear();

        // Get final framebuffer size. This should be the same as the window client area size.
        unsigned int framebuffer_width = 0;
        unsigned int framebuffer_height = 0;
        graphics::get_texture_size(graphics::get_framebuffer_texture(graphics::final_framebuffer),
            framebuffer_width, framebuffer_height);

        const float screen_pixels_per_world_unit = (float)framebuffer_height / GAME_FRAMEBUFFER_HEIGHT;

        // Preallocate the temp_vertices vector.
        {
            size_t vertex_count = 0;
            for (const Text& text : _texts) {
                // In general we will have 4 vertices per non-whitespace glyph.
                // TODO: Handle whitespaces for a more conservative preallocation.
                vertex_count += 4 * text.string.size();
            }
            graphics::temp_vertices.reserve(vertex_count);
        }

        // Create batches.
        for (const Text& text : _texts) {
            fonts::Font* font = fonts::get_font(text.font);
            if (!font)
                continue;

            const Handle<graphics::Texture> texture = fonts::get_atlas_texture(*font);
            if (texture == Handle<graphics::Texture>())
                continue;

            // How many pixels high *on screen* the text should appear.
            const float height_on_screen = text.height * screen_pixels_per_world_unit;
            // How much the glyphs need to be scaled to appear height_on_screen pixels high *on screen*.
            const float scale_for_screen = fonts::get_scale_for_pixel_height(*font, height_on_screen);
            // How much the glyphs need to be scaled to appear text.height units high *in the game world*.
            const float scale_for_world = scale_for_screen / screen_pixels_per_world_unit;

            const int whitespace_advance = fonts::get_whitespace_advance(*font);
            const int line_spacing = fonts::get_line_spacing(*font);

            Batch& batch = _get_new_or_current_batch(texture);

            Vec2f pos; // Aka "pen" or "glyph origin" - current position to draw the glyph at.
            fonts::GlyphId prev_glyph{};

            for (char32_t codepoint : text.string) {
                if (codepoint == U'\r')
                    continue; // Skip carriage returns to avoid graphical issues

                const fonts::GlyphId glyph = fonts::get_glyph(*font, codepoint);

                pos.x += fonts::get_kerning_advance(*font, prev_glyph, glyph);

                switch (codepoint) {
                    case U' ': {
                        pos.x += whitespace_advance;
                    } continue;
                    case U'\t': {
                        pos.x += whitespace_advance * 4.f; // 1 tab = 4 whitespaces
                    } continue;
                    case U'\n': {
                        pos.x = 0.f;
                        pos.y -= line_spacing;
                    } continue;
                }

                const fonts::GlyphBoundingBox box = fonts::get_bounding_box(*font, glyph);
                // PITFALL: It's important to use height_on_screen here!
                const fonts::GlyphTextureRect rect = fonts::get_texture_rect(*font, codepoint, height_on_screen);

                const Vec2f min = pos + box.min;
                const Vec2f max = pos + box.max;
                // PITFALL: We need to flip the texture y-coordinate here because we flip the vertex y-coordinate futher below.
                const Vec2f tex_min = { (float)rect.min.x, (float)rect.max.y }; // SIC: max.y
                const Vec2f tex_max = { (float)rect.max.x, (float)rect.min.y }; // SIC: min.y

                // TODO: triangle strip!!!

                graphics::temp_vertices.emplace_back(Vec2f(min.x, min.y), text.color, Vec2f(tex_min.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, min.y), text.color, Vec2f(tex_max.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(min.x, max.y), text.color, Vec2f(tex_min.x, tex_max.y));
                graphics::temp_vertices.emplace_back(Vec2f(min.x, max.y), text.color, Vec2f(tex_min.x, tex_max.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, min.y), text.color, Vec2f(tex_max.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, max.y), text.color, Vec2f(tex_max.x, tex_max.y));

                for (unsigned int gv = 0; gv < 6; ++gv) { // gv = glyph vertex
                    const unsigned int v = batch.vertex_offset + batch.vertex_count + gv;
                    graphics::Vertex& vertex = graphics::temp_vertices[v];
                     // The glyphs use a coordinate system with y up, so we must flip.
                    vertex.position.y = -vertex.position.y;
                    vertex.position *= scale_for_world;
                    vertex.position += text.position;
                }

                batch.vertex_count += 6;
                pos.x += fonts::get_advance(*font, glyph);
                prev_glyph = glyph;
            }

            if (fonts::atlas_texture_needs_updating(*font)) {
                fonts::update_atlas_texture(*font);
            }
        }

        graphics::set_primitives(graphics::Primitives::TriangleList);
        graphics::bind_vertex_shader(graphics::sprite_vert);
        graphics::bind_fragment_shader(graphics::text_frag);

        graphics::update_or_recreate_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices.data(),
            (unsigned int)graphics::temp_vertices.size() * sizeof(graphics::Vertex));

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