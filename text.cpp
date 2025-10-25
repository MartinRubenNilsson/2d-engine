#include "stdafx.h"
#include "text.h"
#include "text_fonts.h"
#include "text_shaping.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"
#include "graphics_debugging.h"

namespace text {
    Vec2f _get_anchor_position(const Rect2f& box, TextAnchor anchor) {
        const Vec2f box_cen = (box.min + box.max) * 0.5f; // center
        switch (anchor) {
            default: [[fallthrough]]; // Should never happen.
            case TextAnchor::UpperLeft:    return { box.min.x, box.max.y };
            case TextAnchor::UpperCenter:  return { box_cen.x, box.max.y };
            case TextAnchor::UpperRight:   return { box.max.y, box.max.y };
            case TextAnchor::MiddleLeft:   return { box.min.x, box_cen.y };
            case TextAnchor::MiddleCenter: return { box_cen.x, box_cen.y };
            case TextAnchor::MiddleRight:  return { box.max.y, box_cen.y };
            case TextAnchor::LowerLeft:    return { box.min.x, box.min.y };
            case TextAnchor::LowerCenter:  return { box_cen.x, box.min.y };
            case TextAnchor::LowerRight:   return { box.max.y, box.min.y };
        }
    }

    Rect2f get_bounding_box(const Text& text) {
        if (!text.font)
            return Rect2f::EMPTY; // Invalid font.
        text::Font& font = text::get_font(text.font);
        text::TextShape shape{};
        text::shape_text(shape, text.string, font, text.font_size, 0.f, false, false);
        if (shape.glyph_count == 0)
            return Rect2f::EMPTY; // No nonempty glyphs (i.e. nothing is visible).
        shape.bounding_box = sweep(shape.bounding_box, text.shadow_offset);
        const Vec2f anchor_pos = _get_anchor_position(shape.bounding_box, text.anchor);
        const Vec2f translation = text.position - anchor_pos;
        shape.bounding_box = translate(shape.bounding_box, translation);
        return shape.bounding_box;
    }

    std::vector<Text> _texts;

    void draw_later(const Text& text) {
        if (!text.font) return;
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
        unsigned int vertices_begin = 0;
        unsigned int vertices_end = 0; // one-past-the-end
    };

    std::vector<Batch> _batches;

    void draw_all_now() {
        if (_texts.empty())
            return;

        GRAPHICS_DEBUG_GROUP;

        graphics::temp_vertices.clear();
        _batches.clear(); // DEFENSIVE

        // Create batches.
        {
            // Find out how many pixels per world unit we're rendering to.
            const graphics::Viewport& viewport = graphics::get_viewport();
            const float pixels_per_world_unit = viewport.height / GAME_FRAMEBUFFER_HEIGHT;

            TextShape shape{}; // Stored outside the loop to reuse memory.
            for (const Text& text : _texts) {
                if (text.string.empty())
                    continue; // Nothing to draw.

                Font& font = get_font(text.font);

                const Handle<graphics::Texture> texture = get_atlas_texture(font);
                if (texture == Handle<graphics::Texture>())
                    continue; // Nothing to draw.

                shape_text(shape, text.string, font, text.font_size, pixels_per_world_unit, true, true);

                const Vec2f anchor_pos = _get_anchor_position(shape.bounding_box, text.anchor);
                // How much the glyphs need to be translated in order for the anchor to coincide with text.position.
                const Vec2f translation = text.position - anchor_pos;

                // Create vertices for the glyphs.
                for (size_t g = 0; g < shape.glyph_count; ++g) {

                    Rect2f& box = shape.glyph_bounding_boxes[g];
                    box.min += translation;
                    box.max += translation;

                    const Rect2f& rect = shape.glyph_texture_rects[g];

                    // If the text has shadow, add vertices for the shadow glyph first so it renders under the normal glyph.
                    if (text.shadow_offset != Vec2f::ZERO) {
                        const Rect2f& shadow_box = translate(box, text.shadow_offset);
                        graphics::temp_vertices.emplace_back(Vec2f(shadow_box.min.x, shadow_box.min.y), text.shadow_color, Vec2f(rect.min.x, rect.min.y));
                        graphics::temp_vertices.emplace_back(Vec2f(shadow_box.max.x, shadow_box.min.y), text.shadow_color, Vec2f(rect.max.x, rect.min.y));
                        graphics::temp_vertices.emplace_back(Vec2f(shadow_box.min.x, shadow_box.max.y), text.shadow_color, Vec2f(rect.min.x, rect.max.y));
                        graphics::temp_vertices.emplace_back(Vec2f(shadow_box.min.x, shadow_box.max.y), text.shadow_color, Vec2f(rect.min.x, rect.max.y));
                        graphics::temp_vertices.emplace_back(Vec2f(shadow_box.max.x, shadow_box.min.y), text.shadow_color, Vec2f(rect.max.x, rect.min.y));
                        graphics::temp_vertices.emplace_back(Vec2f(shadow_box.max.x, shadow_box.max.y), text.shadow_color, Vec2f(rect.max.x, rect.max.y));
                    }

                    // Add vertices for the normal glyph.
                    graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.min.y), text.color, Vec2f(rect.min.x, rect.min.y));
                    graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), text.color, Vec2f(rect.max.x, rect.min.y));
                    graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), text.color, Vec2f(rect.min.x, rect.max.y));
                    graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), text.color, Vec2f(rect.min.x, rect.max.y));
                    graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), text.color, Vec2f(rect.max.x, rect.min.y));
                    graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.max.y), text.color, Vec2f(rect.max.x, rect.max.y));
                }

                const unsigned int vertices_end = (unsigned int)graphics::temp_vertices.size(); // one-past-the-end

                const Handle<graphics::Sampler> sampler = text.linear_sampling ?
                    graphics::linear_sampler : graphics::nearest_sampler;
                
                if (_batches.empty()) {
                    // Start the first batch.
                    _batches.emplace_back(texture, sampler, 0, vertices_end);
                    continue;
                }
                Batch& current_batch = _batches.back();
                if (current_batch.texture == texture &&
                    current_batch.sampler == sampler) {
                    // Continue the current batch.
                    current_batch.vertices_end = vertices_end;
                    continue;
                }
                // Start the next batch.
                _batches.emplace_back(texture, sampler, current_batch.vertices_end, vertices_end);
            }
        }

        // Update font texture atlases.
        for (const Text& text : _texts) {
            Font& font = get_font(text.font);
            if (atlas_texture_needs_updating(font)) {
                update_atlas_texture(font);
            }
        }

        // At this point we don't need the texts anymore.
        _texts.clear();

        // Update vertex buffer.
        graphics::update_or_recreate_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices);

        // At this point we don't need the temp vertex buffer anymore.
        graphics::temp_vertices.clear();

        // Draw all batches.
        graphics::set_primitives(graphics::Primitives::TriangleList);
        graphics::bind_vertex_buffer(0, graphics::dynamic_vertex_buffer, sizeof(graphics::VertexPCT));
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
                const unsigned int vertex_count = batch.vertices_end - batch.vertices_begin;
                const unsigned int vertex_offset = batch.vertices_begin;
                graphics::draw(vertex_count, vertex_offset);
            }
        }

        // At this point we're done with the batches.
        _batches.clear();
    }
}