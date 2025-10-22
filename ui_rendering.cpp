#include "stdafx.h"
#include "ui_data.h"
#include "text_fonts.h"
#include "text_shaping.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_debugging.h"
#include <clay/clay.h>

namespace ui {
	Handle<graphics::VertexShader> _ui_vert;
	Handle<graphics::FragmentShader> _ui_frag;

	void _load_shaders() {
		_ui_vert = graphics::load_vertex_shader("assets/shaders/ui_clay.vert");
		_ui_frag = graphics::load_fragment_shader("assets/shaders/ui_clay.frag");
	}

	struct Batch {
		Handle<graphics::Texture> texture{};
		unsigned int vertices_begin = 0;
		unsigned int vertices_end = 0; // one-past-the-end
	};

	std::vector<Batch> _batches;
	std::vector<text::FontId> _fonts_to_update; // fonts whose atlas texture needs updating

	void _add_rectangle_vertices(const Rect2f& box, Color color, const Rect2f& tex_rect = Rect2f::ZERO) {
		graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.min.y), color, Vec2f(tex_rect.min.x, tex_rect.min.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(tex_rect.max.x, tex_rect.min.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(tex_rect.min.x, tex_rect.max.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(tex_rect.min.x, tex_rect.max.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(tex_rect.max.x, tex_rect.min.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.max.y), color, Vec2f(tex_rect.max.x, tex_rect.max.y));
	}

	void _add_border_vertices(const Rect2f& box, Color color, float left, float right, float top, float bottom) {
		if (left > 0.f) { // left border
			_add_rectangle_vertices({ box.min, { box.min.x + left, box.max.y } }, color);
		}
		if (right > 0.f) { // right border
			_add_rectangle_vertices({ { box.max.x - right, box.min.y }, box.max}, color);
		}
		if (top > 0.f) { // top border
			_add_rectangle_vertices({ { box.min.x + left, box.min.y }, { box.max.x - right, box.min.y + top } }, color);
		}
		if (bottom > 0.f) { // bottom border
			_add_rectangle_vertices({ { box.min.x + left, box.max.y - bottom }, { box.max.x - right, box.max.y } }, color);
		}
	}

	void _render_clay(const Clay_Dimensions& dimensions, std::span<const Clay_RenderCommand> commands) {
		if (dimensions.width <= 0.f || dimensions.height <= 0.f)
			return; // DEFENSIVE
		if (commands.empty())
			return; // So we don't do unneccesary work rendering nothing at all.

		GRAPHICS_DEBUG_GROUP;

		const graphics::Viewport& viewport = graphics::get_viewport();
		// How many pixels we're rendering to per layout unit.
		const float pixels_per_unit = viewport.height / dimensions.height;

		graphics::temp_vertices.clear();
		_batches.clear();
		_fonts_to_update.clear();

		// Create batches.
		for (const Clay_RenderCommand& command : commands) {

			Rect2f box{}; // Bounding box.
			box.min.x = command.boundingBox.x;
			box.min.y = command.boundingBox.y;
			box.max.x = command.boundingBox.x + command.boundingBox.width;
			box.max.y = command.boundingBox.y + command.boundingBox.height;

			// Snap to pixels.
			// PITFALL: This may cause slight discrepancies between the visual bounding box and the
			// bounding box used for input (mouse hover, etc.), but this shouldn't be a big problem.
			{
				const Vec2f new_min = round(box.min);
				const Vec2f offset = new_min - box.min;
				box.min = new_min;
				box.max += offset;
			}

			Handle<graphics::Texture> texture = graphics::white_texture; // 1x1 white texture

			switch (command.commandType) {
				case CLAY_RENDER_COMMAND_TYPE_NONE: {

					// This command type should be skipped.

				} break;
				case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {

					const Clay_RectangleRenderData& data = command.renderData.rectangle;

					Color color = data.backgroundColor;
					if (color == Color(0, 0, 0, 0)) // PITFALL: this is the default color for some commands
						color = Color::WHITE;

					_add_rectangle_vertices(box, color);

					// TODO: corner radius

				} break;
				case CLAY_RENDER_COMMAND_TYPE_BORDER: {

					const Clay_BorderRenderData& data = command.renderData.border;

					Color color = data.color;
					if (color == Color(0, 0, 0, 0)) // PITFALL: this is the default color for some commands
						color = Color::WHITE;

					_add_border_vertices(box, color, data.width.left, data.width.right, data.width.top, data.width.bottom);

					// TODO: corner radius

				} break;
				case CLAY_RENDER_COMMAND_TYPE_TEXT: {

					const Clay_TextRenderData& data = command.renderData.text;
					const std::string_view string{ data.stringContents.chars, (size_t)data.stringContents.length };
					if (string.empty())
						continue; // Nothing to render.

					const text::FontId font_id{ .id = data.fontId };
					if (!font_id)
						continue; // Invalid font.

					Color color = data.textColor;
					if (color == Color(0, 0, 0, 0)) // PITFALL: this is the default color for some commands
						color = Color::WHITE;

					text::Font& font = text::get_font(font_id);
					texture = text::get_atlas_texture(font);

					static text::TextShape shape{}; // Static so we can reuse the buffers.
					text::shape_text(shape, string, font, data.fontSize, pixels_per_unit, true, true);

					// How much the text needs to be translated for the UI bounding box and text bounding box to conincide.
					const Vec2f translation = box.min - shape.bounding_box.min;

					Vec2f shadow_offset{};
					Color shadow_color{};
					if (command.userData) {
						const TextData& text_data = *(const TextData*)command.userData;
						shadow_offset = text_data.shadow_offset;
						shadow_color = text_data.shadow_color;
					}

					// PITFALL: There's no need to sweep the box to account for the shadow offset, because _measure_text()
					// will already have done so. If we sweep it here we will double-sweep. So translation is correct as-is.
					
					// PITFALL 2: Having the bounding box account for the shadow caused problems with pixel alignments, so
					// I've turned it off in _measure_text(). As long as the shadow has a small offset this shouldn't cause
					// any problems.

					// Create vertices for the glyphs.
					for (size_t g = 0; g < shape.glyph_count; ++g) {

						Rect2f& box = shape.glyph_bounding_boxes[g];
						box.min += translation;
						box.max += translation;

						Rect2f& rect = shape.glyph_texture_rects[g];
						// HACK: We use negative texture coordinates to indicate that the texture is grayscale
						// (only has one channel), as for example is the case for font atlas textures.
						rect.min = -rect.min;
						rect.max = -rect.max;

						// If the text has shadow, add vertices for the shadow glyph first so it renders under the normal glyph.
						if (shadow_offset != Vec2f::ZERO) {
							const Rect2f& shadow_box = translate(box, shadow_offset);
							_add_rectangle_vertices(shadow_box, shadow_color, rect);
						}

						// Add vertices for the normal glyph.
						_add_rectangle_vertices(box, color, rect);
					}

					// Check if font atlas texture needs to be updated.
					if (text::atlas_texture_needs_updating(font)) {
						bool already_in_fonts_to_update = false;
						for (const text::FontId font_to_update : _fonts_to_update) {
							if (font_to_update == font_id) {
								already_in_fonts_to_update = true;
								break;
							}
						}
						if (!already_in_fonts_to_update) {
							_fonts_to_update.push_back(font_id);
						}
					}

				} break;
				case CLAY_RENDER_COMMAND_TYPE_IMAGE: {

					const Clay_ImageRenderData& data = command.renderData.image;
					if (!data.imageData) continue; // DEFENSIVE

					const ImageData& image_data = *(const ImageData*)data.imageData;
					texture = image_data.texture;

					const Vec2f texture_size = graphics::get_texture_size(image_data.texture);

					Rect2f rect{}; // texture rect in UV-space (normalized coordinates)
					rect.min = (Vec2f)image_data.position / texture_size;
					rect.max = rect.min + (Vec2f)image_data.size / texture_size;

					Color color = data.backgroundColor;
					if (color == Color(0, 0, 0, 0)) // PITFALL: this is the default color for some commands
						color = Color::WHITE;

					_add_rectangle_vertices(box, color, rect);

				} break;
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {

					// The renderer should begin clipping all future draw commands, only rendering content that falls within the provided boundingBox.
					//console::log_error("CLAY_RENDER_COMMAND_TYPE_SCISSOR_START is not supported"); // TODO
					continue;

				} break;
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {

					// The renderer should finish any previously active clipping, and begin rendering elements in full again.
					//console::log_error("CLAY_RENDER_COMMAND_TYPE_SCISSOR_END is not supported"); // TODO
					continue;

				} break;
				case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {

					continue;

				} break;
			}

			const unsigned vertices_end = (unsigned int)graphics::temp_vertices.size(); // one-past-the-end

			if (_batches.empty()) {
				// Start the first batch.
				_batches.emplace_back(texture, 0, vertices_end);
				continue;
			}
			Batch& current_batch = _batches.back();
			if (current_batch.texture == texture) {
				// Continue the current batch.
				current_batch.vertices_end = vertices_end;
				continue;
			}
			// Start the next batch.
			_batches.emplace_back(texture, current_batch.vertices_end, vertices_end);
		}

		// Transform all vertices to clip space.
		for (unsigned int v = 0; v < graphics::temp_vertices.size(); ++v) {
			graphics::Vertex& vertex = graphics::temp_vertices[v];
			vertex.position.x /= dimensions.width;
			vertex.position.y /= dimensions.height;
			vertex.position.x = vertex.position.x * 2.f - 1.f;
			vertex.position.y = vertex.position.y * 2.f - 1.f;
		}

		// Update and bind the vertex buffer.
		graphics::update_or_recreate_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices.data(),
			(unsigned int)graphics::temp_vertices.size() * sizeof(graphics::Vertex));
		graphics::bind_vertex_buffer(0, graphics::dynamic_vertex_buffer, sizeof(graphics::Vertex));

		// At this point we're done with the temp vertex buffer.
		graphics::temp_vertices.clear();

		// Update all font atlas textures that need to be updated.
		for (const text::FontId font_id : _fonts_to_update) {
			text::Font& font = text::get_font(font_id);
			text::update_atlas_texture(font);
		}

		// At this point we're done updating the font atlas textures.
		_fonts_to_update.clear();

		// Draw all batches.
		graphics::set_primitives(graphics::Primitives::TriangleList);
		graphics::bind_vertex_shader(_ui_vert);
		graphics::bind_fragment_shader(_ui_frag);
		graphics::bind_sampler(0, graphics::nearest_sampler);
		for (const Batch& batch : _batches) {
			graphics::bind_texture(0, batch.texture);
			const unsigned int vertex_count = batch.vertices_end - batch.vertices_begin;
			const unsigned int vertex_offset = batch.vertices_begin;
			graphics::draw(vertex_count, vertex_offset);
		}

		// At this point we're done with the batches.
		_batches.clear();
	}
}