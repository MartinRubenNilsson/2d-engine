#include "stdafx.h"
#include "ui_clay.h"
#include "window_events.h"
#include "console.h"
#include "graphics.h"
#include "graphics_globals.h" // TODO: don't put this here
#include "graphics_debugging.h"

#include "text.h"
#include "text_fonts.h"
#include "text_shaping.h"
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

#if 0
					const Clay_TextRenderData& data = command.renderData.text;
					const std::string_view string{ data.stringContents.chars, (size_t)data.stringContents.length };
					const text::FontId font_id{ .id = data.fontId };
					if (!font_id)
						continue; // Invalid font.
					text::Font& font = text::get_font(font_id);
					text::shape_text(text_shape, string, font, data.fontSize, pixels_per_unit, true, true);
					// TODO: create vertices
#endif

				} break;
				case CLAY_RENDER_COMMAND_TYPE_IMAGE: {

					const Clay_ImageRenderData& data = command.renderData.image;
					if (!data.imageData) continue; // DEFENSIVE

					const ImageData& image_data = *(const ImageData*)data.imageData;
					const Vec2f texture_size = graphics::get_texture_size(image_data.texture);

					Rect2f rect{}; // texture rect in UV-space (normalized coordinates)
					rect.min = (Vec2f)image_data.rect_position / texture_size;
					rect.max = rect.min + (Vec2f)image_data.rect_size / texture_size;

					Color color = data.backgroundColor;
					if (color == Color(0, 0, 0, 0)) // PITFALL: this is the default color for some commands
						color = Color::WHITE;

					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.min.y), color, Vec2f(rect.min.x, rect.min.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(rect.max.x, rect.min.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(rect.min.x, rect.max.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(rect.min.x, rect.max.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(rect.max.x, rect.min.y));
					graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.max.y), color, Vec2f(rect.max.x, rect.max.y));

					texture = image_data.texture;

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