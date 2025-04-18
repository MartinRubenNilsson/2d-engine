#include "stdafx.h"
#include "text.h"
#include "fonts.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"

namespace text {
    void render(const Text& text) {
        if (text.unicode_string.empty()) return;

        float whitespace_width = (float)fonts::get_glyph(text.font, U' ').advance_width;
        const float letter_spacing = whitespace_width * (text.letter_spacing_factor - 1.f);
        whitespace_width += letter_spacing;
        const float line_spacing = fonts::get_line_spacing(text.font) * text.line_spacing_factor;
        Vector2f current_point; // In unscaled coordinates

        std::vector<graphics::Vertex> vertices;

        char32_t previous_codepoint = 0;
        for (char32_t codepoint : text.unicode_string) {
            if (codepoint == U'\r') continue; // Skip carriage returns to avoid graphical issues

            current_point.x += fonts::get_kerning_advance(text.font, previous_codepoint, codepoint);

            switch (codepoint) {
            case U' ': {
                current_point.x += whitespace_width;
            } continue; // Don't need to create a quad for whitespaces
            case U'\t': {
                current_point.x += whitespace_width * 4.f;
            } continue; // Don't need to create a quad for whitespaces
            case U'\n': {
                current_point.x = 0.f;
                current_point.y += line_spacing;
            } continue; // Don't need to create a quad for whitespaces
            }

            const fonts::Glyph glyph = fonts::get_glyph(text.font, codepoint);

            const Vector2f pos0 = current_point + Vector2f((float)glyph.x0, (float)glyph.y0);
            const Vector2f pos1 = current_point + Vector2f((float)glyph.x1, (float)glyph.y1);
            Vector2f tex0 = Vector2f((float)glyph.s0, (float)glyph.t0) / (float)fonts::ATLAS_TEXTURE_SIZE;
            Vector2f tex1 = Vector2f((float)glyph.s1, (float)glyph.t1) / (float)fonts::ATLAS_TEXTURE_SIZE;
            std::swap(tex0.y, tex1.y); // Flip y-axis
            vertices.emplace_back(Vector2f(pos0.x, pos0.y), colors::WHITE, Vector2f(tex0.x, tex0.y));
            vertices.emplace_back(Vector2f(pos1.x, pos0.y), colors::WHITE, Vector2f(tex1.x, tex0.y));
            vertices.emplace_back(Vector2f(pos0.x, pos1.y), colors::WHITE, Vector2f(tex0.x, tex1.y));
            vertices.emplace_back(Vector2f(pos0.x, pos1.y), colors::WHITE, Vector2f(tex0.x, tex1.y));
            vertices.emplace_back(Vector2f(pos1.x, pos0.y), colors::WHITE, Vector2f(tex1.x, tex0.y));
            vertices.emplace_back(Vector2f(pos1.x, pos1.y), colors::WHITE, Vector2f(tex1.x, tex1.y));

            current_point.x += glyph.advance_width + letter_spacing;
            previous_codepoint = codepoint;
        }

        const float scale_for_pixel_height = fonts::get_scale_for_pixel_height(text.font, text.pixel_height);
        for (graphics::Vertex& vertex : vertices) {
            vertex.position.y = -vertex.position.y; // Flip y-axis
            vertex.position *= scale_for_pixel_height;
            vertex.position *= text.scale;
            vertex.position += text.position;
        }

        graphics::bind_vertex_shader(graphics::sprite_vert);
        graphics::bind_fragment_shader(graphics::text_frag);
        graphics::bind_texture(0, fonts::get_atlas_texture(text.font));
        graphics::update_buffer(graphics::dynamic_vertex_buffer,
            vertices.data(), (unsigned int)vertices.size() * sizeof(graphics::Vertex));
        graphics::set_primitives(graphics::Primitives::TriangleList);
        graphics::draw((unsigned int)vertices.size());
    }
}