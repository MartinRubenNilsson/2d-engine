#include "stdafx.h"
#include "window_events.h"
#include "console.h"
#include "sprites.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "ui_hud.h"

#pragma warning(push)
#pragma warning(disable: 4244) // conversion from '...' to '...', possible loss of data
#define CLAY_IMPLEMENTATION
#include <clay/clay.h>
#include "ui_clay.h"
#pragma warning(pop)

namespace ui {
	const Clay_Color COLOR_LIGHT = Clay_Color{ 224, 215, 210, 255 };
	const Clay_Color COLOR_RED = Clay_Color{ 168, 66, 28, 255 };
	const Clay_Color COLOR_ORANGE = Clay_Color{ 225, 138, 50, 255 };

	void _handle_clay_error(Clay_ErrorData error_data) {
		std::string_view text{ error_data.errorText.chars, (size_t)error_data.errorText.length };
		console::log_error(text);
	}

	std::vector<uint8_t> _clay_arena_memory;
	Clay_Arena _clay_arena{};
	Clay_RenderCommandArray _clay_render_commands{};

	bool startup_clay() {
		_clay_arena_memory.resize(Clay_MinMemorySize());
		_clay_arena = Clay_CreateArenaWithCapacityAndMemory(_clay_arena_memory.size(), _clay_arena_memory.data());
		const Clay_Dimensions dimensions{ .width = GAME_FRAMEBUFFER_WIDTH, .height = GAME_FRAMEBUFFER_HEIGHT };
		const Clay_ErrorHandler error_handler{ .errorHandlerFunction = _handle_clay_error };
		return Clay_Initialize(_clay_arena, dimensions, error_handler);
	}

	void shutdown_clay() {
		_clay_render_commands = {};
		_clay_arena = {};
		_clay_arena_memory.clear();
	}

	void set_clay_pointer_state(float x, float y, bool is_down) {
		// TODO: If in the future we render the game world to a smaller viewport, then
		// we need to remap the mouse coordinates to the smaller viewport.
		Clay_SetPointerState({ .x = x, .y = y }, is_down);
	}

	void update_clay(float dt) {
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
				case window::EventType::MouseScroll: {
					const Clay_Vector2 scoll_delta{
						.x = (float)ev.mouse_scroll.delta_x,
						.y = (float)ev.mouse_scroll.delta_y };
					Clay_UpdateScrollContainers(false, scoll_delta, dt);
				} break;
			}
		}
	}

	void layout_clay() {
		_clay_render_commands = {};
		Clay_BeginLayout();
		hud::layout();
		_clay_render_commands = Clay_EndLayout();
	}

	void render_clay() {
		if (!_clay_render_commands.length)
			return;

		graphics::ScopedDebugGroup debug_group(__FUNCTION__);

		for (int32_t i = 0; i < _clay_render_commands.length; ++i) {
			const Clay_RenderCommand& command = _clay_render_commands.internalArray[i];
			switch (command.commandType) {
				case CLAY_RENDER_COMMAND_TYPE_NONE: {
					// This command type should be skipped.
				} break;
				case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
					// The renderer should draw a solid color rectangle.
					const Clay_RectangleRenderData& rectangle = command.renderData.rectangle;
					sprites::Sprite sprite{};
					sprite.vertex_shader = graphics::ui_rectangle_vert;
					sprite.fragment_shader = graphics::ui_rectangle_frag;
					sprite.position.x = command.boundingBox.x;
					sprite.position.y = command.boundingBox.y;
					sprite.size.x = command.boundingBox.width;
					sprite.size.y = command.boundingBox.height;
					sprite.color = {
						(unsigned char)rectangle.backgroundColor.r,
						(unsigned char)rectangle.backgroundColor.g,
						(unsigned char)rectangle.backgroundColor.b,
						(unsigned char)rectangle.backgroundColor.a
					};
					sprites::draw_later(sprite);
					sprites::draw_all_now(); // TODO: optimize
				} break;
				case CLAY_RENDER_COMMAND_TYPE_BORDER: {
					// The renderer should draw a colored border inset into the bounding box.
					__debugbreak();
				} break;
				case CLAY_RENDER_COMMAND_TYPE_TEXT: {
					// The renderer should draw text.
					__debugbreak();
				} break;
				case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
					// The renderer should draw an image.
					//__debugbreak(); //TODO
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
					__debugbreak();
				} break;
			}
		}
	}
}