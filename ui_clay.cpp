#include "stdafx.h"
#include "ui_clay.h"
#include "window_events.h"
#include "console.h"
#include "sprites.h"
#include "graphics.h"
#include "graphics_globals.h" // TODO: don't put this here

#include "text.h"
#include "ui_hud.h" // TODO: don't put this here

#pragma warning(push)
#pragma warning(disable: 4244) // conversion from '...' to '...', possible loss of data
#define CLAY_IMPLEMENTATION
#include <clay/clay.h>
#pragma warning(pop)

namespace ui {
	void _handle_clay_error(Clay_ErrorData error_data) {
		std::string_view text{ error_data.errorText.chars, (size_t)error_data.errorText.length };
		console::log_error(text);
	}

	Clay_Dimensions _measure_text(Clay_StringSlice string, Clay_TextElementConfig* config, void* userData) {
		text::Text text{};
		text.font = { .id = config->fontId };
		text.font_size = config->fontSize;
		// TODO: copying the string like this is baaaaad!!!
		text.string = { string.chars, (size_t)string.length };
		const Rect2f rect = text::get_bounding_box(text);
		const Vec2f size = rect.max - rect.min;
		return { size.x, size.y };
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

	void _add_quad_vertices(const Rect2f& box, const Rect2f& uv, const Color& color) {
		graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.min.y), color, Vec2f(uv.min.x, uv.min.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(uv.max.x, uv.min.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(uv.min.x, uv.max.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.min.x, box.max.y), color, Vec2f(uv.min.x, uv.max.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.min.y), color, Vec2f(uv.max.x, uv.min.y));
		graphics::temp_vertices.emplace_back(Vec2f(box.max.x, box.max.y), color, Vec2f(uv.max.x, uv.max.y));
	}

	void _transform_vertices_to_clip_space(const Clay_Dimensions& dimensions) {
		for (graphics::Vertex& v : graphics::temp_vertices) {
			v.position.x /= dimensions.width;
			v.position.y /= dimensions.height;
			v.position.x = v.position.x * 2.f - 1.f;
			v.position.y = v.position.y * 2.f - 1.f;
		}
	}

	void _update_and_bind_vertex_buffer() {
		graphics::update_or_recreate_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices.data(),
			(unsigned int)graphics::temp_vertices.size() * sizeof(graphics::Vertex));
		graphics::bind_vertex_buffer(0, graphics::dynamic_vertex_buffer, sizeof(graphics::Vertex));
		graphics::temp_vertices.clear();
	}

	void render() {
		if (!_clay_render_commands.length)
			return;

		const graphics::ScopedDebugGroup debug_group(__FUNCTION__);

		const graphics::Viewport& viewport = graphics::get_viewport();
		const Clay_Dimensions dimensions = Clay_GetCurrentContext()->layoutDimensions;
		// How many pixels we're rendering to per layout unit.
		const float pixels_per_unit = viewport.height / dimensions.height;

		const std::span<const Clay_RenderCommand> commands{ // This is just to make it easier to debug.
			_clay_render_commands.internalArray, (size_t)_clay_render_commands.length };

		graphics::temp_vertices.clear();

		graphics::set_primitives(graphics::Primitives::TriangleList);
		graphics::bind_vertex_shader(_ui_clay_vert);
		graphics::bind_fragment_shader(_ui_clay_frag);
		graphics::bind_sampler(0, graphics::nearest_sampler);

		for (const Clay_RenderCommand& command : commands) {

			Rect2f box{};
			box.min.x = command.boundingBox.x;
			box.min.y = command.boundingBox.y;
			box.max.x = command.boundingBox.x + command.boundingBox.width;
			box.max.y = command.boundingBox.y + command.boundingBox.height;

			switch (command.commandType) {
				case CLAY_RENDER_COMMAND_TYPE_NONE: {
					// This command type should be skipped.
				} break;
				case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
					const Clay_RectangleRenderData& data = command.renderData.rectangle;
					const Color color = data.backgroundColor;
					_add_quad_vertices(box, Rect2f::ZERO, color);
					_transform_vertices_to_clip_space(dimensions);
					_update_and_bind_vertex_buffer();
					graphics::bind_texture(0, graphics::white_texture);
					graphics::draw(6); // draw 1 quad = 2 tris = 6 verts
				} break;
				case CLAY_RENDER_COMMAND_TYPE_BORDER: {
					// The renderer should draw a colored border inset into the bounding box.
					__debugbreak(); // TODO
				} break;
				case CLAY_RENDER_COMMAND_TYPE_TEXT: {
#if 0
					const Clay_TextRenderData& data = command.renderData.text;
					text::Text text{};
					text.position.x = command.boundingBox.x;
					text.position.y = command.boundingBox.y;
					text.font.id = data.fontId;
					text.font_size = (float)data.fontSize;
					// TODO: don't copy the string
					text.string = { data.stringContents.chars, (size_t)data.stringContents.length };
					text.color = data.textColor;
					if (text.color == Color(0, 0, 0, 0)) // PITFALL: this is the default color
						text.color = Color::WHITE;
					text.linear_sampling = false;
					text.anchor = text::TextAnchor::UpperLeft;
					text::draw_later(text);
					text::draw_all_now(__FUNCTION__); // TODO: camera is all wrong
#endif
				} break;
				case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
					const Clay_ImageRenderData& data = command.renderData.image;
					if (!data.imageData) continue; // DEFENSIVE
					const Image& image = *(const Image*)data.imageData;
					const Vec2f texture_size = graphics::get_texture_size(image.texture);
					Rect2f uv{};
					uv.min = (Vec2f)image.tex_rect_pos / texture_size;
					uv.max = uv.min + (Vec2f)image.tex_rect_size / texture_size;
					Color color = data.backgroundColor;
					if (color == Color(0, 0, 0, 0)) // PITFALL: this is the default backgroundColor for images
						color = Color::WHITE;
					_add_quad_vertices(box, uv, color);
					_transform_vertices_to_clip_space(dimensions);
					_update_and_bind_vertex_buffer();
					graphics::bind_texture(0, image.texture);
					graphics::draw(6); // draw 1 quad = 2 tris = 6 verts
				} break;
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
					// The renderer should begin clipping all future draw commands, only rendering content that falls within the provided boundingBox.
					__debugbreak(); // TODO
				} break;
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
					// The renderer should finish any previously active clipping, and begin rendering elements in full again.
					__debugbreak(); // TODO
				} break;
				case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
					// The renderer should provide a custom implementation for handling this render command based on its .customData
					console::log_error("CLAY_RENDER_COMMAND_TYPE_CUSTOM is not supported");
				} break;
			}
		}


	}
}