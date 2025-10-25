#include "stdafx.h"
#include "ui.h"
#include "ui_data.h"
#include "ui_game.h"
#include "window.h"
#include "window_events.h"
#include "text_fonts.h"
#include "text_shaping.h"
#include "console.h"

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
		// PITFALL: Clay calls this function with parts of the user-provided string (for example, it measures
		// whitespaces separately) and uses the result to build the bounding box. It has proven quite hard to
		// make the result match the bounding box produced by text::shape_text(), which measures a whole text.
		const std::string_view string{ text.chars, (size_t)text.length };
		const text::FontId font_id{ .id = config->fontId };
		if (!font_id) {
			console::log_error("UI error: Invalid font ID = " + std::to_string(font_id.id) + ". Text = " + std::string(string));
			return { 0.f, 0.f };
		}
		if (config->fontSize <= 0.f) {
			console::log_error("UI error: Invalid font size. Text = " + std::string(string));
			return { 0.f, 0.f };
		}
		text::Font& font = text::get_font(font_id);
		text::TextShape shape{};
		text::shape_text(shape, string, font, config->fontSize, 0.f, true, false);
#if 0 // Grow the box to account for shadows. This is causing problems with pixel alignments, so I've turned it off.
		if (config->userData) {
			const TextData& data = *(const TextData*)config->userData;
			shape.bounding_box = sweep(shape.bounding_box, data.shadow_offset); 
		}
#endif
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

	void _load_shaders(); // ui_rendering.cpp

	void startup() {
		_startup_clay();
		_load_shaders();
		game::startup();
	}

	void shutdown() {
		game::shutdown();
		_shutdown_clay();
	}

	bool _contains(std::span<const uint32_t> ids, const uint32_t& id) {
		for (const uint32_t& other_id : ids) {
			if (other_id == id) {
				return true;
			}
		}
		return false;
	}

	bool debug = false;

	enum class MouseState {
		Released,
		ReleasedThisFrame,
		Pressed,
		PressedThisFrame,
	};

	MouseState _mouse_state = MouseState::Released;
	std::vector<uint32_t> _mouse_over_ids;
	std::vector<uint32_t> _mouse_enter_ids;
	std::vector<uint32_t> _mouse_leave_ids;

	void update(float dt) {
#if 1
		Clay_SetDebugModeEnabled(debug);
#endif

		static Clay_Vector2 mouse_pos{};

		if (_mouse_state == MouseState::PressedThisFrame) {
			_mouse_state = MouseState::Pressed;
		}
		if (_mouse_state == MouseState::ReleasedThisFrame) {
			_mouse_state = MouseState::Released;
		}

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
					// The window size will generally be much bigger than the layout size,
					// so we need to transform the mouse position to match the layout.
					const Vec2i window_size = window::get_size();
					if (window_size.x == 0 || window_size.y == 0)
						break;
					mouse_pos.x = ((float)ev.mouse_move.x / window_size.x) * GAME_FRAMEBUFFER_WIDTH;
					mouse_pos.y = ((float)ev.mouse_move.y / window_size.y) * GAME_FRAMEBUFFER_HEIGHT;
				} break;
				case window::EventType::MouseButtonPress: {
					if (ev.mouse_button.button == window::MouseButton::Left) {
						_mouse_state = MouseState::PressedThisFrame;
					}
				} break;
				case window::EventType::MouseButtonRelease: {
					if (ev.mouse_button.button == window::MouseButton::Left) {
						_mouse_state = MouseState::ReleasedThisFrame;
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

		bool is_mouse_down = (_mouse_state == MouseState::PressedThisFrame || _mouse_state == MouseState::Pressed);

		// Make sure Clay doesn't capture the mouse if ImGui is already doing so.
		if (ImGui::GetIO().WantCaptureMouse) {
			mouse_pos.x = -1.f;
			mouse_pos.y = -1.f;
			is_mouse_down = false;
		}

		// This updates the array returned by Clay_GetPointerOverIds() and calls any callbacks set
		// with Clay_OnHover() (among other things).
		Clay_SetPointerState(mouse_pos, is_mouse_down);

		// Update mouse over/enter/leave IDs.
		_mouse_enter_ids.clear();
		_mouse_leave_ids.clear();
		{
			static std::vector<uint32_t> curr_mouse_over_ids;
			{
				const Clay_ElementIdArray ids = Clay_GetPointerOverIds();
				for (int32_t i = 0; i < ids.length; ++i) {
					curr_mouse_over_ids.push_back(ids.internalArray[i].id);
				}
			}
			for (uint32_t prev_id : _mouse_over_ids) {
				if (!_contains(curr_mouse_over_ids, prev_id)) {
					_mouse_leave_ids.push_back(prev_id);
				}
			}
			for (uint32_t curr_id : curr_mouse_over_ids) {
				if (!_contains(_mouse_over_ids, curr_id)) {
					_mouse_enter_ids.push_back(curr_id);
				}
			}
			std::swap(_mouse_over_ids, curr_mouse_over_ids);
			curr_mouse_over_ids.clear();
		}

		game::update(dt);
	}

	Clay_RenderCommandArray _clay_render_commands{};

	void layout() {
		_clay_render_commands = {};
		Clay_BeginLayout();
		game::layout();
		if (debug) {
#if 0
			static Clay_CustomRenderData _DEBUG_START_CUSTOM_DATA{};
			// HACK: Use the custom render command as a separator between the "normal" commands
			// and the commands generated by the Clay debugger.
			CLAY(CLAY_ID("DebugStart"), { .custom = { .customData = &_DEBUG_START_CUSTOM_DATA } });
#endif
		}
		_clay_render_commands = Clay_EndLayout();
	}

	void _render_clay(const Clay_Dimensions& dimensions, std::span<const Clay_RenderCommand> commands); // ui_rendering.cpp

	void render() {
		const Clay_Dimensions dimensions = Clay_GetCurrentContext()->layoutDimensions;
		_render_clay(dimensions, { _clay_render_commands.internalArray, (size_t)_clay_render_commands.length });
	}

	bool mouse_down() {
		return _mouse_state == MouseState::PressedThisFrame;
	}

	bool mouse_pressed() {
		return _mouse_state == MouseState::PressedThisFrame || _mouse_state == MouseState::Pressed;
	}

	bool mouse_up() {
		return _mouse_state == MouseState::ReleasedThisFrame;
	}

	bool mouse_over() {
		return Clay_Hovered();
	}

	bool mouse_enter() {
		const uint32_t id = Clay__GetOpenLayoutElementId();
		return _contains(_mouse_enter_ids, id);
	}

	bool mouse_leave() {
		const uint32_t id = Clay__GetOpenLayoutElementId();
		return _contains(_mouse_leave_ids, id);
	}
}