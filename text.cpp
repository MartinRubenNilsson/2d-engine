#include "stdafx.h"
#include "text.h"
#include "text_fonts.h"
#include "text_unicode.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"

namespace text {
    // Sorts by draw order.
    bool operator<(const Text& a, const Text& b) {
        if (a.font != b.font)
            return a.font < b.font;
        return a.linear_sampling < b.linear_sampling; // is this correct?
    }

    std::vector<Text> _texts;

    void draw_later(const Text& text) {
        _texts.push_back(text);
    }

    struct Batch {
        Handle<graphics::Texture> texture{};
        Handle<graphics::Sampler> sampler{};
        unsigned int vertex_offset = 0;
        unsigned int vertex_count = 0;
    };

    std::vector<Batch> _batches;

    Batch& _get_new_or_current_batch(Handle<graphics::Texture> texture, Handle<graphics::Sampler> sampler) {
        // Do we need to create a new batch?
        if (_batches.empty() || _batches.back().texture != texture || _batches.back().sampler != sampler)
            return _batches.emplace_back(texture, sampler, (unsigned int)graphics::temp_vertices.size());
        // We can continue the current batch.
        return _batches.back();
    }

    void draw_all_now(std::string_view debug_group_name) {
        if (_texts.empty())
            return;

        const graphics::ScopedDebugGroup debug_group(debug_group_name);

        // Sort the texts by draw order.
        std::sort(_texts.begin(), _texts.end());

        // Find out how many screen pixels there are per world unit.
        unsigned int framebuffer_width = 0;
        unsigned int framebuffer_height = 0;
        graphics::get_texture_size(graphics::get_framebuffer_texture(graphics::final_framebuffer),
            framebuffer_width, framebuffer_height);
        const float screen_pixels_per_world_unit = (float)framebuffer_height / GAME_FRAMEBUFFER_HEIGHT;

        // Preallocate storage for the vertices.
        graphics::temp_vertices.clear();
        {
            size_t vertex_count = 0;
            for (const Text& text : _texts) {
                // In general we will have 6 vertices per non-whitespace glyph.
                // TODO: Handle whitespaces for a more conservative preallocation.
                vertex_count += 6 * length(text.string);
            }
            graphics::temp_vertices.reserve(vertex_count);
        }

        // Create batches.
        _batches.clear();
        for (const Text& text : _texts) {
            Font* font = get_font(text.font);
            if (!font) continue;

            const Handle<graphics::Texture> texture = get_atlas_texture(*font);
            if (texture == Handle<graphics::Texture>())
                continue; // Nothing to draw.

            // How many pixels high *on screen* the text should appear.
            const float height_on_screen = text.letter_height * screen_pixels_per_world_unit;
            // How much the glyphs need to be scaled to appear height_on_screen pixels high *on screen*.
            const float scale_for_screen = get_scale_for_pixel_height(*font, height_on_screen);
            // How much the glyphs need to be scaled to appear text.height units high *in the game world*.
            const float scale_for_world = scale_for_screen / screen_pixels_per_world_unit;

            const int whitespace_advance = get_whitespace_advance(*font);
            const int line_spacing = get_line_spacing(*font);

            const unsigned int vertex_offset = (unsigned int)graphics::temp_vertices.size(); // The first vertex for this text.
            unsigned int vertex_count = 0; // How many vertices this text has added.

            Vec2f glyph_origin; // Aka "current position" or "pen". This will move as glyphs are being drawn.
            GlyphId prev_glyph{};

            std::u8string_view string = text.string;
            while (char32_t codepoint = to_c32(string)) {
                if (codepoint == U'\r')
                    continue; // Skip carriage returns

                const GlyphId glyph = get_glyph(*font, codepoint);

                glyph_origin.x += get_kerning_advance(*font, prev_glyph, glyph);

                switch (codepoint) {
                    case U' ': {
                        glyph_origin.x += whitespace_advance;
                    } continue;
                    case U'\t': {
                        glyph_origin.x += whitespace_advance * 4.f; // 1 tab = 4 whitespaces
                    } continue;
                    case U'\n': {
                        glyph_origin.x = 0.f;
                        glyph_origin.y -= line_spacing;
                    } continue;
                }

                const GlyphBoundingBox box = get_bounding_box(*font, glyph);
                const Vec2f min = glyph_origin + box.min;
                const Vec2f max = glyph_origin + box.max;

                // PITFALL: It's important to use height_on_screen here!
                const GlyphTextureRect rect = get_texture_rect(*font, glyph, height_on_screen);
                // PITFALL: We need to flip the texture y-coordinate here because we flip the vertex y-coordinate futher below.
                const Vec2f tex_min = { (float)rect.min.x, (float)rect.max.y }; // SIC: max.y
                const Vec2f tex_max = { (float)rect.max.x, (float)rect.min.y }; // SIC: min.y

                graphics::temp_vertices.emplace_back(Vec2f(min.x, min.y), text.color, Vec2f(tex_min.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, min.y), text.color, Vec2f(tex_max.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(min.x, max.y), text.color, Vec2f(tex_min.x, tex_max.y));
                graphics::temp_vertices.emplace_back(Vec2f(min.x, max.y), text.color, Vec2f(tex_min.x, tex_max.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, min.y), text.color, Vec2f(tex_max.x, tex_min.y));
                graphics::temp_vertices.emplace_back(Vec2f(max.x, max.y), text.color, Vec2f(tex_max.x, tex_max.y));

                vertex_count += 6;
                glyph_origin.x += get_advance(*font, glyph);
                prev_glyph = glyph;
            }

            // Transform the vertices in the text.
            for (unsigned int v = 0; v < vertex_count; ++v) {
                graphics::Vertex& vertex = graphics::temp_vertices[vertex_offset + v];
                // PITFALL: The glyphs use a coordinate system with y up, so we must flip.
                vertex.position.y = -vertex.position.y;
                vertex.position *= scale_for_world;
                vertex.position += text.position;
            }

            // Update the batch.
            const Handle<graphics::Sampler> sampler = text.linear_sampling ?
                graphics::linear_sampler : graphics::nearest_sampler;
            Batch& batch = _get_new_or_current_batch(texture, sampler);
            batch.vertex_count += vertex_count;
        }

        // Update font texture atlases.
        for (const Text& text : _texts) {
            Font* font = get_font(text.font);
            if (!font) continue;
            if (atlas_texture_needs_updating(*font)) {
                update_atlas_texture(*font);
            }
        }
        _texts.clear(); // At this point we don't need the texts anymore.

        // Update vertex buffer.
        graphics::update_or_recreate_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices.data(),
            (unsigned int)graphics::temp_vertices.size() * sizeof(graphics::Vertex));
        graphics::temp_vertices.clear(); // At this point we don't need the temp buffer anymore.

        // Draw all batches.
        graphics::set_primitives(graphics::Primitives::TriangleList);
        graphics::bind_vertex_buffer(0, graphics::dynamic_vertex_buffer, sizeof(graphics::Vertex));
        graphics::bind_vertex_shader(graphics::sprite_vert);
        graphics::bind_fragment_shader(graphics::text_frag);
        {
            Handle<graphics::Texture> prev_texture{};
            Handle<graphics::Sampler> prev_sampler{};
            for (const Batch& batch : _batches) {
                if (batch.texture != prev_texture) {
                    graphics::bind_texture(0, batch.texture);
                    prev_texture = batch.texture;
                }
                if (batch.sampler != prev_sampler) {
                    graphics::bind_sampler(0, batch.sampler);
                    prev_sampler = batch.sampler;
                }
                graphics::draw(batch.vertex_count, batch.vertex_offset);
            }
        }
        _batches.clear(); // At this point we're done with the batches.
    }
}