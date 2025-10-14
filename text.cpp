#include "stdafx.h"
#include "text.h"
#include "text_fonts.h"
#include "text_unicode.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"

namespace text {
    Vec2f _get_anchor_position(const Vec2f& min, const Vec2f& max, TextAnchor anchor) {
        const Vec2f cen = (min + max) * 0.5f; // center
        switch (anchor) {
            case TextAnchor::UpperLeft:    return { min.x, max.y };
            case TextAnchor::UpperCenter:  return { cen.x, max.y };
            case TextAnchor::UpperRight:   return { max.y, max.y };
            case TextAnchor::MiddleLeft:   return { min.x, cen.y };
            case TextAnchor::MiddleCenter: return { cen.x, cen.y };
            case TextAnchor::MiddleRight:  return { max.y, cen.y };
            case TextAnchor::LowerLeft:    return { min.x, min.y };
            case TextAnchor::LowerCenter:  return { cen.x, min.y };
            case TextAnchor::LowerRight:   return { max.y, min.y };
            default: return cen; // Should never happen.
        }
    }

    // Calculates the bounding box of the text and, optionally, creates vertices.
    // Pass a negative value for pixels_per_world_unit to skip creating vertices.
    Rect2f _shape_text(Font& font, const Text& text, float pixels_per_world_unit = -1.f) {

        // The pixel resolution (in number of pixels from ascender to descender) the glyphs will have.
        // This will usually be much larger than text.font_size since there are many pixels on screen
        // per world-space length unit.
        const float font_pixel_size = text.font_size * pixels_per_world_unit;

        const int whitespace_advance = get_whitespace_advance(font); // How much to horizontally advance past a whitespace.
        const int line_spacing = get_line_spacing(font); // How much to vertically advance on a newline.

        Vec2f glyph_pos; // Aka "current position" or "pen". This is the origin for where to draw the next glyph.
        GlyphId prev_glyph{};
        Rect2f text_box = Rect2f::EMPTY; // Bounding box for the entire text (in text local-space).

        const size_t vertex_count_before = graphics::temp_vertices.size();

        std::u8string_view string = text.string; // This will shink as the codepoints are being decoded.
        while (char32_t codepoint = to_c32(string)) { // Decode the next codepoint.
            if (codepoint == U'\r')
                continue; // Skip carriage returns.

            const GlyphId glyph = get_glyph(font, codepoint);

            glyph_pos.x += get_kerning_advance(font, prev_glyph, glyph);

            switch (codepoint) {
                case U' ': {
                    glyph_pos.x += whitespace_advance;
                } continue;
                case U'\t': {
                    glyph_pos.x += whitespace_advance * 4.f; // 1 tab = 4 whitespaces
                } continue;
                case U'\n': {
                    glyph_pos.x = 0.f;
                    glyph_pos.y += line_spacing; // Move the origin *down* a line.
                } continue;
            }

            Rect2f box = get_bounding_box(font, glyph);
            // PITFALL: The glyphs use a local coordinate system with positive y up, while our game world use a
            // coordinate system with positive y down, so we need to flip the sign here.
            box.min.y = -box.min.y;
            box.max.y = -box.max.y;
            // PITFALL: We must also swap the min.y and max.y, since the above sign flip has caused min.y > max.y.
            std::swap(box.min.y, box.max.y);
            // Translate the glyph box to the pen position.
            box.min += glyph_pos;
            box.max += glyph_pos;

            // Expand the text bounding box to include the bounding box of the glyph.
            text_box = join(text_box, box);

            if (font_pixel_size > 0.f && !empty(font, glyph)) { // only add vertices for glyphs that have something to render

                // PITFALL: It's vital to use font_pixel_size here and not just text.font_size! Otherwise the pixel density
                // of the glyphs will not match their size on screen and they will appear to have very low resolution.
                Rect2f rect = get_texture_rect(font, glyph, font_pixel_size);

                graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.min.y), text.color, Vec2f(rect.min.x, rect.min.y));
                graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), text.color, Vec2f(rect.max.x, rect.min.y));
                graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), text.color, Vec2f(rect.min.x, rect.max.y));
                graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), text.color, Vec2f(rect.min.x, rect.max.y));
                graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), text.color, Vec2f(rect.max.x, rect.min.y));
                graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.max.y), text.color, Vec2f(rect.max.x, rect.max.y));
            }

            glyph_pos.x += get_advance(font, glyph);
            prev_glyph = glyph;
        }

        const size_t vertex_count_after = graphics::temp_vertices.size();

        // How much the glyphs need to be scaled in order to appear text.font_size units high in world-space.
        const float scale = get_scale_for_font_size(font, text.font_size);

        // Scale the text bounding box so it has the correct world-space size.
        text_box.min *= scale;
        text_box.max *= scale;

        // How much the glyphs need to be translated in order for the anchor to coincide with text.position.
        const Vec2f translation = text.position - _get_anchor_position(text_box.min, text_box.max, text.anchor);

        // Translate the text bounding box so it has the correct world-space position.
        text_box.min += translation;
        text_box.max += translation;

        // Transform any new vertices to have the desired world-space scale and position.
        for (size_t v = vertex_count_before; v < vertex_count_after; ++v) {
            graphics::Vertex& vertex = graphics::temp_vertices[v];
            vertex.position *= scale;
            vertex.position += translation;
        }

        return text_box;
    }

    Rect2f get_bounding_box(const Text& text) {
        Font* font = get_font(text.font);
        if (!font) return Rect2f::EMPTY;
        return _shape_text(*font, text);
    }

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

    void _add_to_batches(Handle<graphics::Texture> texture, Handle<graphics::Sampler> sampler, unsigned int vertex_count) {
        if (_batches.empty()) {
            _batches.emplace_back(texture, sampler, 0, vertex_count); // Create the first batch.
        } else if (_batches.back().texture == texture || _batches.back().sampler == sampler) {
            _batches.back().vertex_count += vertex_count; // Continue the current batch.
        } else {
            const unsigned int vertex_offset = _batches.back().vertex_offset + _batches.back().vertex_count;
            _batches.emplace_back(texture, sampler, vertex_offset, vertex_count); // Create the next batch.
        }
    }

    void draw_all_now(std::string_view debug_group_name) {
        if (_texts.empty())
            return;

        const graphics::ScopedDebugGroup debug_group(debug_group_name);

        // Find out how many screen pixels there are per world unit.
        const Vec2u framebuffer_size = graphics::get_texture_size(
            graphics::get_framebuffer_texture(graphics::final_framebuffer));
        const float pixels_per_world_unit = (float)framebuffer_size.y / GAME_FRAMEBUFFER_HEIGHT;

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


            const unsigned int vertex_count_before = (unsigned int)graphics::temp_vertices.size();

            _shape_text(*font, text, pixels_per_world_unit);

            const unsigned int vertex_count_after = (unsigned int)graphics::temp_vertices.size();

            const Handle<graphics::Sampler> sampler = text.linear_sampling ?
                graphics::linear_sampler : graphics::nearest_sampler;

            _add_to_batches(texture, sampler, vertex_count_after - vertex_count_before);
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