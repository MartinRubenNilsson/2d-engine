#include "stdafx.h"
#include "ui.h"
#include "console.h"
#include "sprites.h"
#include "graphics.h"
#include "graphics_globals.h"

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

	std::string_view _clay_string_to_string_view(const Clay_String& string) {
		return std::string_view{ string.chars, (size_t)string.length };
	}

	void _handle_clay_errors(Clay_ErrorData error_data) {
		console::log_error(_clay_string_to_string_view(error_data.errorText));
	}

	std::vector<uint8_t> _clay_arena_memory;
	Clay_Arena _clay_arena{};
	Clay_RenderCommandArray _clay_render_commands{};

	bool initialize_clay() {
		_clay_arena_memory.resize(Clay_MinMemorySize());
		_clay_arena = Clay_CreateArenaWithCapacityAndMemory(
			(uint32_t)_clay_arena_memory.size(),
			_clay_arena_memory.data()
		);
		return Clay_Initialize(
			_clay_arena, {
				.width = GAME_FRAMEBUFFER_WIDTH,
				.height = GAME_FRAMEBUFFER_HEIGHT,
			}, {
				.errorHandlerFunction = _handle_clay_errors,
			}
		);
	}

	void shutdown_clay() {
		_clay_render_commands = {};
		_clay_arena = {};
		_clay_arena_memory.clear();
	}

	void set_clay_layout_dimensions(float width, float height) {
		Clay_SetLayoutDimensions({ .width = width, .height = height });
	}

	void set_clay_pointer_state(float x, float y, bool is_down) {
		Clay_SetPointerState({ .x = x, .y = y }, is_down);
	}

	void update_clay_scroll_containers(float scroll_delta_x, float dt) {
		Clay_UpdateScrollContainers(false, { .x = scroll_delta_x }, dt);
	}

	void begin_clay_layout() {
		_clay_render_commands = {};
		Clay_BeginLayout();
	}

	// Example measure text function
	static inline Clay_Dimensions MeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, uintptr_t userData) {
		// Clay_TextElementConfig contains members such as fontId, fontSize, letterSpacing etc
		// Note: Clay_String->chars is not guaranteed to be null terminated
		return {
			.width = (float)text.length * config->fontSize, // <- this will only work for monospace fonts, see the renderers/ directory for more advanced text measurement
				.height = (float)config->fontSize
		};
	}

	// Layout config is just a struct that can be declared statically, or inline
	Clay_ElementDeclaration sidebarItemConfig = {
		.layout = {
			.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50) }
		},
		.backgroundColor = COLOR_ORANGE
	};

	// Re-useable components are just normal functions
	void SidebarItemComponent() {
		CLAY(sidebarItemConfig) {
			// children go here...
		}
	}

	void _test_clay() {
		// An example of laying out a UI with a fixed width sidebar and flexible width main content
		CLAY({
			.id = CLAY_ID("OuterContainer"),
			.layout = {
				.sizing = {
					CLAY_SIZING_GROW(0),
					CLAY_SIZING_GROW(0)
				},
				.padding = CLAY_PADDING_ALL(16),
				.childGap = 16
			},
			.backgroundColor = {250,250,255,255}
		}) {
			CLAY({
				.id = CLAY_ID("SideBar"),
				.layout = {
					.sizing = {
						.width = CLAY_SIZING_FIXED(300),
						.height = CLAY_SIZING_GROW(0)
					},
					.padding = CLAY_PADDING_ALL(16),
					.childGap = 16,
					.layoutDirection = CLAY_TOP_TO_BOTTOM
				},
				.backgroundColor = COLOR_LIGHT
			}) {
				CLAY({
					.id = CLAY_ID("ProfilePictureOuter"),
					.layout = {
						.sizing = {
							.width = CLAY_SIZING_GROW(0)
						},
						.padding = CLAY_PADDING_ALL(16),
						.childGap = 16,
						.childAlignment = {
							.y = CLAY_ALIGN_Y_CENTER
						}
					},
					.backgroundColor = COLOR_RED
				}) {

#if 0
					static int imageData = 0;

					CLAY({
						.id = CLAY_ID("ProfilePicture"),
						.layout = {
							.sizing = {
								.width = CLAY_SIZING_FIXED(60),
								.height = CLAY_SIZING_FIXED(60)
							}
						},
						.image = {
							.imageData = &imageData,
							.sourceDimensions = {60, 60}
						}
					}) {}
#endif

#if 0
					CLAY_TEXT(
						CLAY_STRING("Clay - UI Library"),
						CLAY_TEXT_CONFIG({
							.textColor = {255, 255, 255, 255},
							.fontSize = 24
						})
					);
#endif
				}

				// Standard C code like loops etc work inside components
				for (int i = 0; i < 5; i++) {
					SidebarItemComponent();
				}

				CLAY({
					.id = CLAY_ID("MainContent"),
					.layout = {
						.sizing = {
							.width = CLAY_SIZING_GROW(0),
							.height = CLAY_SIZING_GROW(0)
						}
					},
					.backgroundColor = COLOR_LIGHT
				}) {}
			}
		}
	}

	void end_clay_layout() {
		_clay_render_commands = Clay_EndLayout();
	}

	void render_clay_layout() {
		return;
		if (!_clay_render_commands.length) return;
		graphics::ScopedDebugGroup debug_group("ui::render_clay_layout()");
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
					sprites::draw_sprite_later(sprite);
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
		// No need to sort, since the Clay render commands are already sorted.
		sprites::draw_all_sprites();
	}
}