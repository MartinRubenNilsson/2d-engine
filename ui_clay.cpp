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
		text.string.assign((const char8_t*)string.chars, string.length);
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

	Handle<graphics::VertexShader> _image_vert{};
	Handle<graphics::FragmentShader> _image_frag{};

	void _load_shaders() {
		_image_vert = graphics::load_vertex_shader("assets/shaders/ui_image.vert");
		_image_frag = graphics::load_fragment_shader("assets/shaders/ui_image.frag");
	}

	void startup() {
		_startup_clay();
		_load_shaders();
		hud::startup(); // TODO: move somewhere else
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

	void render() {
		if (!_clay_render_commands.length)
			return;

		const graphics::ScopedDebugGroup debug_group(__FUNCTION__);

		const graphics::Viewport& viewport = graphics::get_viewport();
		const float pixels_per_world_unit = (float)viewport.height / GAME_FRAMEBUFFER_HEIGHT;

		const std::span<const Clay_RenderCommand> commands{ // This is just to make it easier to debug.
			_clay_render_commands.internalArray, (size_t)_clay_render_commands.length };

		for (const Clay_RenderCommand& command : commands) {
			switch (command.commandType) {
				case CLAY_RENDER_COMMAND_TYPE_NONE: {
					// This command type should be skipped.
				} break;
				case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
					const Clay_RectangleRenderData& data = command.renderData.rectangle;
					sprites::Sprite sprite{};
					sprite.vertex_shader = graphics::ui_rectangle_vert;
					sprite.fragment_shader = graphics::ui_rectangle_frag;
					sprite.position.x = command.boundingBox.x * pixels_per_world_unit;
					sprite.position.y = command.boundingBox.y * pixels_per_world_unit;
					sprite.size.x = command.boundingBox.width * pixels_per_world_unit;
					sprite.size.y = command.boundingBox.height * pixels_per_world_unit;
					sprite.color = data.backgroundColor;
					sprites::draw_later(sprite);
					sprites::draw_all_now(__FUNCTION__); // TODO: optimize
				} break;
				case CLAY_RENDER_COMMAND_TYPE_BORDER: {
					// The renderer should draw a colored border inset into the bounding box.
					__debugbreak();
				} break;
				case CLAY_RENDER_COMMAND_TYPE_TEXT: {
					// The renderer should draw text.
					// TODO
				} break;
				case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
					const Clay_ImageRenderData& data = command.renderData.image;
					const Image& image = *(const Image*)data.imageData;
					sprites::Sprite sprite{};
					sprite.vertex_shader = _image_vert;
					sprite.fragment_shader = _image_frag;
					sprite.texture = image.texture;
					sprite.position.x = command.boundingBox.x * pixels_per_world_unit;
					sprite.position.y = command.boundingBox.y * pixels_per_world_unit;
					sprite.size.x = command.boundingBox.width * pixels_per_world_unit;
					sprite.size.y = command.boundingBox.height * pixels_per_world_unit;
					sprite.tex_position = image.tex_rect_pos;
					sprite.tex_size = image.tex_rect_size;
					const Vec2f texture_size = graphics::get_texture_size(image.texture);
					sprite.tex_position /= texture_size;
					sprite.tex_size /= texture_size;
					sprite.color = data.backgroundColor;
					if (sprite.color == Color(0, 0, 0, 0)) // untinted
						sprite.color = Color::WHITE;
					sprites::draw_later(sprite);
					sprites::draw_all_now(__FUNCTION__); // TODO: optimize
				} break;
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
					// The renderer should begin clipping all future draw commands, only rendering content that falls within the provided boundingBox.
					__debugbreak();
				} break;
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
					// The renderer should finish any previously active clipping, and begin rendering elements in full again.
					__debugbreak();
				} break;
				case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
					// The renderer should provide a custom implementation for handling this render command based on its .customData
					console::log_error("CLAY_RENDER_COMMAND_TYPE_CUSTOM is not supported");
				} break;
			}
		}
	}
}