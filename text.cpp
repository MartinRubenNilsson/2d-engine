#include "stdafx.h"
#include "text.h"
#include "text_fonts.h"
#include "text_unicode.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"

namespace text {
    std::vector<Text> _texts;

    void draw_later(const Text& text) {
        _texts.push_back(text);
    }

    // Orders the texts by draw order.
    bool operator<(const Text& a, const Text& b) {
        if (a.font != b.font)
            return a.font < b.font;
        return a.linear_sampling < b.linear_sampling; // is this correct?
    }

    void sort_all() {
        std::sort(_texts.begin(), _texts.end());
    }

    struct Batch {
        Handle<graphics::Texture> texture{};
        Handle<graphics::Sampler> sampler{};
        unsigned int vertex_offset = 0;
        unsigned int vertex_count = 0;
    };

    std::vector<Batch> _batches;

    Batch& _get_new_or_current_batch(Handle<graphics::Texture> texture, Handle<graphics::Sampler> sampler) {
        if (_batches.empty())
            return _batches.emplace_back(texture, sampler); // Create the first batch
        if (_batches.back().texture == texture || _batches.back().sampler == sampler)
            return _batches.back(); // Continue the current batch
        return _batches.emplace_back(texture, sampler, _batches.back().vertex_offset + _batches.back().vertex_count);
    }

    Vec2f _get_origin(const Vec2f& min, const Vec2f& max, TextOrigin origin) {
        const Vec2f cen = (min + max) * 0.5f; // center
        switch (origin) {
            default: return Vec2f::ZERO;
            case TextOrigin::UpperLeft:    return { min.x, max.y };
            case TextOrigin::UpperCenter:  return { cen.x, max.y };
            case TextOrigin::UpperRight:   return { max.y, max.y };
            case TextOrigin::MiddleLeft:   return { min.x, cen.y };
            case TextOrigin::MiddleCenter: return { cen.x, cen.y };
            case TextOrigin::MiddleRight:  return { max.y, cen.y };
            case TextOrigin::LowerLeft:    return { min.x, min.y };
            case TextOrigin::LowerCenter:  return { cen.x, min.y };
            case TextOrigin::LowerRight:   return { max.y, min.y };
        }
    }

    void draw_all_now(std::string_view debug_group_name) {
        if (_texts.empty())
            return;

        const graphics::ScopedDebugGroup debug_group(debug_group_name);

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
            if (text.string.empty())
                continue; // Nothing to draw.

            const Handle<graphics::Texture> texture = get_atlas_texture(*font);
            if (texture == Handle<graphics::Texture>())
                continue; // Nothing to draw.

            // How many pixels high *on screen* the text should appear.
            const float font_size_on_screen = text.font_size * screen_pixels_per_world_unit;

            Vec2f text_min = { FLT_MAX, FLT_MAX }; // Bounding box min for the entire text (in text local-space).
            Vec2f text_max = { -FLT_MAX, -FLT_MAX }; // Bounding box max for the entire text (in text local-space).
            const unsigned int vertex_offset = (unsigned int)graphics::temp_vertices.size(); // The first vertex for this text.
            unsigned int vertex_count = 0; // How many vertices this text has added.
            {
                const int whitespace_advance = get_whitespace_advance(*font);
                const int line_spacing = get_line_spacing(*font);

                Vec2f glyph_origin; // Aka "current position" or "pen". This will move as glyphs are being drawn.
                GlyphId prev_glyph{};
                std::u8string_view string = text.string; // This will shrink as the codepoints are being decoded.

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
                            glyph_origin.y += line_spacing; // Move the origin *down* a line.
                        } continue;
                    }

                    GlyphBoundingBox box = get_bounding_box(*font, glyph);
                    // PITFALL: The glyphs use a local coordinate system with positive y up, while our game world use a
                    // coordinate system with positive y down, so we need to flip the sign here.
                    box.min.y = -box.min.y;
                    box.max.y = -box.max.y;
                    const Vec2f min = glyph_origin + box.min;
                    const Vec2f max = glyph_origin + box.max;

                    // Update text bounding box limits.
                    text_min = ::min(text_min, min);
                    text_max = ::max(text_max, max);

                    // PITFALL: It's important to use height_on_screen here!
                    GlyphTextureRect rect = get_texture_rect(*font, glyph, font_size_on_screen);
                    // PITFALL: I'm not sure why we need to do this to be honest...
                    std::swap(rect.min.y, rect.max.y);

                    graphics::temp_vertices.emplace_back(Vec2f(min.x, min.y), text.color, Vec2f(rect.min.x, rect.min.y));
                    graphics::temp_vertices.emplace_back(Vec2f(max.x, min.y), text.color, Vec2f(rect.max.x, rect.min.y));
                    graphics::temp_vertices.emplace_back(Vec2f(min.x, max.y), text.color, Vec2f(rect.min.x, rect.max.y));
                    graphics::temp_vertices.emplace_back(Vec2f(min.x, max.y), text.color, Vec2f(rect.min.x, rect.max.y));
                    graphics::temp_vertices.emplace_back(Vec2f(max.x, min.y), text.color, Vec2f(rect.max.x, rect.min.y));
                    graphics::temp_vertices.emplace_back(Vec2f(max.x, max.y), text.color, Vec2f(rect.max.x, rect.max.y));

                    vertex_count += 6;
                    glyph_origin.x += get_advance(*font, glyph); // Advance the origin to the next glyph.
                    prev_glyph = glyph;
                }

                // How much the glyphs need to be scaled to appear height_on_screen pixels high *on screen*.
                const float scale_for_screen = get_scale_for_pixel_height(*font, font_size_on_screen);
                // How much the glyphs need to be scaled to appear text.height units high *in the game world*.
                const float scale_for_world = scale_for_screen / screen_pixels_per_world_unit;

                // Determine the world-space position of the origin.
                const Vec2f origin = _get_origin(text_min, text_max, text.origin) * scale_for_world;
                const Vec2f position = text.position - origin;

                // Transform the vertices in the text.
                for (unsigned int v = 0; v < vertex_count; ++v) {
                    graphics::Vertex& vertex = graphics::temp_vertices[vertex_offset + v];
                    vertex.position *= scale_for_world;
                    vertex.position += position;
                }

                // Update the batch.
                const Handle<graphics::Sampler> sampler = text.linear_sampling ?
                    graphics::linear_sampler : graphics::nearest_sampler;
                _get_new_or_current_batch(texture, sampler).vertex_count += vertex_count;
            }
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