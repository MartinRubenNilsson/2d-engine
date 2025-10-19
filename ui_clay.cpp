#include "stdafx.h"
#include "ui_clay.h"
#include "window_events.h"
#include "text_fonts.h"
#include "text_shaping.h"
#include "console.h"
#include "graphics.h"
#include "graphics_globals.h" // TODO: don't put this here
#include "graphics_debugging.h"
#include "ui_hud.h" // TODO: don't put this here

#pragma warning(push)
#pragma warning(disable: 4244) // conversion from '...' to '...', possible loss of data
#define CLAY_IMPLEMENTATION
#include <clay/clay.h>
#pragma warning(pop)

namespace ui {
	void _handle_clay_error(Clay_ErrorData error_data) {
		const std::string_view text{ error_data.errorText.chars, (size_t)error_data.errorText.length };
		console::log_error(text);
	}

	Clay_Dimensions _measure_text(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
		const std::string_view string{ text.chars, (size_t)text.length };
		const text::FontId font_id{ .id = config->fontId };
		if (!font_id)
			return { 0.f, 0.f }; // Invalid font ID.
		text::Font& font = text::get_font(font_id);
		text::TextShape shape{};
		text::shape_text(shape, string, font, config->fontSize, 0.f, false, false);
		if (shape.glyph_count == 0)
			return { 0.f, 0.f }; // No nonempty glyphs (i.e. nothing is visible).
		if (config->userData) {
			const TextData& data = *(const TextData*)config->userData;
			shape.bounding_box = sweep(shape.bounding_box, data.shadow_offset); // grow the box to account for shadows
		}
		const Vec2f box_size = shape.bounding_box.max - shape.bounding_box.min;
		return { box_size.x, box_size.y };
	}

	std::vector<uint8_t> _clay_arena_memory;
	Clay_Arena _clay_arena{};

	void _startup_clay() {
		_clay_arena_memory.resize(Clay_MinMemorySize());
		_clay_arena = Clay_CreateArenaWithCapacityAndMemory(_clay_arena_memory.size(), _clay_arena_memory.data());
		const Clay_Dimensions dimensions{ .width = GAME_FRAMEBUFFER_WIDTH, .height = GAME_FRAMEBUFFER_HEIGHT };
		const Clay_ErrorHandler error_handler{ .errorHandlerFunction = _handle_clay_error };
		Clay_Initialize(_clay_arena, dimensions, error_handler);
		Clay_SetMeasureTextFunction(_measure_text, nullptr);
	}

	void _shutdown_clay() {
		_clay_arena = {};
		_clay_arena_memory = {};
	}

	Handle<graphics::VertexShader> _ui_clay_vert;
	Handle<graphics::FragmentShader> _ui_clay_frag;

	void _load_shaders() {
		_ui_clay_vert = graphics::load_vertex_shader("assets/shaders/ui_clay.vert");
		_ui_clay_frag = graphics::load_fragment_shader("assets/shaders/ui_clay.frag");
	}

	void startup() {
		_startup_clay();
		_load_shaders();
		hud::startup();
	}

	void shutdown() {
		_shutdown_clay();
	}

	void update(float dt) {
		static Clay_Vector2 mouse_pos{};
		static bool mouse_is_down = false;

		for (const window::Event& ev : window::get_events()) {
			switch (ev.type) {
#if 0
				case window::EventType::FramebufferSize: {
					// TODO: If in the future we render the game world to a smaller viewport, then
					// we should probably still pass the gameworld size here???
					const Clay_Dimensions dimensions{
						.width = (float)ev.size.width,
						.height = (float)ev.size.height };
					Clay_SetLayoutDimensions(dimensions);
				} break;
#endif
				case window::EventType::MouseMove: {
					mouse_pos.x = (float)ev.mouse_move.x;
					mouse_pos.y = (float)ev.mouse_move.y;
				} break;
				case window::EventType::MouseButtonPress: {
					if (ev.mouse_button.button == window::MouseButton::Left) {
						mouse_is_down = true;
					}
				} break;
				case window::EventType::MouseButtonRelease: {
					if (ev.mouse_button.button == window::MouseButton::Left) {
						mouse_is_down = false;
					}
				} break;
				case window::EventType::MouseScroll: {
					const Clay_Vector2 scoll_delta{
						.x = (float)ev.mouse_scroll.delta_x,
						.y = (float)ev.mouse_scroll.delta_y };
					Clay_UpdateScrollContainers(false, scoll_delta, dt);
				} break;
			}
		}

		if (ImGui::GetIO().WantCaptureMouse)
			mouse_is_down = false;

		Clay_SetPointerState(mouse_pos, mouse_is_down);

		hud::update(dt); // TODO: move to like game_ui.h or someting
	}

	Clay_RenderCommandArray _clay_render_commands{};

	void layout() {
		_clay_render_commands = {};
		Clay_BeginLayout();
		hud::layout(); // TODO: move to like game_ui.h or someting
		_clay_render_commands = Clay_EndLayout();
	}

	struct Batch {
		Handle<graphics::Texture> texture{};
		unsigned int vertices_begin = 0;
		unsigned int vertices_end = 0; // one-past-the-end
	};

	std::vector<Batch> _batches;
	std::vector<text::FontId> _fonts_to_update; // fonts whose atlas texture needs updating

	void render() {
		if (!_clay_render_commands.length)
			return;

		GRAPHICS_DEBUG_GROUP;

		const graphics::Viewport& viewport = graphics::get_viewport();
		const Clay_Dimensions dimensions = Clay_GetCurrentContext()->layoutDimensions;
		// How many pixels we're rendering to per layout unit.
		const float pixels_per_unit = viewport.height / dimensions.height;

		const std::span<const Clay_RenderCommand> commands{ // This is just to make it easier to debug.
			_clay_render_commands.internalArray, (size_t)_clay_render_commands.length };

		graphics::temp_vertices.clear();
		_batches.clear();
		_fonts_to_update.clear();
		
		// Create batches.

		text::TextShape text_shape{}; // Stored outside the loop so we can reuse the memory.
		for (const Clay_RenderCommand& command : commands) {

			Rect2f box{}; // Bounding box.
			box.min.x = command.boundingBox.x;
			box.min.y = command.boundingBox.y;
			box.max.x = command.boundingBox.x + command.boundingBox.width;
			box.max.y = command.boundingBox.y + command.boundingBox.height;

			Handle<graphics::Texture> texture = graphics::white_texture;

			switch (command.commandType) {
				case CLAY_RENDER_COMMAND_TYPE_NONE: {

					// This command type should be skipped.

				} break;
				case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {

					const Clay_RectangleRenderData& data = command.renderData.rectangle;

					Color color = data.backgroundColor;
					if (color == Color(0, 0, 0, 0)) // PITFALL: this is the default color for some commands
						color = Color::WHITE;

					// Add vertices for the quad.
					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.min.y), color, Vec2f::ZERO);
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f::ZERO);
					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f::ZERO);
					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f::ZERO);
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f::ZERO);
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.max.y), color, Vec2f::ZERO);

					// TODO: corner radius

				} break;
				case CLAY_RENDER_COMMAND_TYPE_BORDER: {

					// The renderer should draw a colored border inset into the bounding box.
					console::log_error("CLAY_RENDER_COMMAND_TYPE_BORDER is not supported"); // TODO
					continue;

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

					text::shape_text(text_shape, string, font, data.fontSize, pixels_per_unit, true, true);

					// How much the text needs to be translated for the UI bounding box and text bounding box to conincide.
					const Vec2f translation = box.min - text_shape.bounding_box.min;

					Vec2f shadow_offset{};
					Color shadow_color{};
					if (command.userData) {
						const TextData& text_data = *(const TextData*)command.userData;
						shadow_offset = text_data.shadow_offset;
						shadow_color = text_data.shadow_color;
					}

					// PITFALL: There's no need to sweep the box to account for the shadow offset, because _measure_text()
					// will already have done so. If we sweep it here we will double-sweep. So translation is correct as-is.

					// Create vertices for the glyphs.
					for (size_t g = 0; g < text_shape.glyph_count; ++g) {

						Rect2f& box = text_shape.glyph_bounding_boxes[g];
						box.min += translation;
						box.max += translation;

						Rect2f& rect = text_shape.glyph_texture_rects[g];
						// HACK: We use negative texture coordinates to indicate that the texture is grayscale
						// (only has one channel), as for example is the case for font atlas textures.
						rect.min = -rect.min;
						rect.max = -rect.max;

						// If the text has shadow, add vertices for the shadow glyph first so it renders under the normal glyph.
						if (shadow_offset != Vec2f::ZERO) {
							const Rect2f& shadow_box = translate(box, shadow_offset);
							graphics::temp_vertices.emplace_back(Vec2f(shadow_box.min.x, shadow_box.min.y), shadow_color, Vec2f(rect.min.x, rect.min.y));
							graphics::temp_vertices.emplace_back(Vec2f(shadow_box.max.x, shadow_box.min.y), shadow_color, Vec2f(rect.max.x, rect.min.y));
							graphics::temp_vertices.emplace_back(Vec2f(shadow_box.min.x, shadow_box.max.y), shadow_color, Vec2f(rect.min.x, rect.max.y));
							graphics::temp_vertices.emplace_back(Vec2f(shadow_box.min.x, shadow_box.max.y), shadow_color, Vec2f(rect.min.x, rect.max.y));
							graphics::temp_vertices.emplace_back(Vec2f(shadow_box.max.x, shadow_box.min.y), shadow_color, Vec2f(rect.max.x, rect.min.y));
							graphics::temp_vertices.emplace_back(Vec2f(shadow_box.max.x, shadow_box.max.y), shadow_color, Vec2f(rect.max.x, rect.max.y));
						}

						// Add vertices for the normal glyph.
						graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.min.y), color, Vec2f(rect.min.x, rect.min.y));
						graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(rect.max.x, rect.min.y));
						graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(rect.min.x, rect.max.y));
						graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(rect.min.x, rect.max.y));
						graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(rect.max.x, rect.min.y));
						graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.max.y), color, Vec2f(rect.max.x, rect.max.y));
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
					rect.min = (Vec2f)image_data.rect_position / texture_size;
					rect.max = rect.min + (Vec2f)image_data.rect_size / texture_size;

					Color color = data.backgroundColor;
					if (color == Color(0, 0, 0, 0)) // PITFALL: this is the default color for some commands
						color = Color::WHITE;

					// Create vertices for the image.
					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.min.y), color, Vec2f(rect.min.x, rect.min.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(rect.max.x, rect.min.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(rect.min.x, rect.max.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(rect.min.x, rect.max.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(rect.max.x, rect.min.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.max.y), color, Vec2f(rect.max.x, rect.max.y));

				} break;
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {

					// The renderer should begin clipping all future draw commands, only rendering content that falls within the provided boundingBox.
					console::log_error("CLAY_RENDER_COMMAND_TYPE_SCISSOR_START is not supported"); // TODO
					continue;

				} break;
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {

					// The renderer should finish any previously active clipping, and begin rendering elements in full again.
					console::log_error("CLAY_RENDER_COMMAND_TYPE_SCISSOR_END is not supported"); // TODO
					continue;

				} break;
				case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {

					// The renderer should provide a custom implementation for handling this render command based on its .customData
					console::log_error("CLAY_RENDER_COMMAND_TYPE_CUSTOM is not supported"); // TODO
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
		for (graphics::Vertex& v : graphics::temp_vertices) {
			v.position.x /= dimensions.width;
			v.position.y /= dimensions.height;
			v.position.x = v.position.x * 2.f - 1.f;
			v.position.y = v.position.y * 2.f - 1.f;
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
		graphics::bind_vertex_shader(_ui_clay_vert);
		graphics::bind_fragment_shader(_ui_clay_frag);
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