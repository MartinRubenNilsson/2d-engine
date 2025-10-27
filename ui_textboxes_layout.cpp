#include "stdafx.h"
#include "ui_textboxes.h"
#include "ui.h"
#include "ui_shared.h"

namespace ui {
namespace textboxes {

	const Clay_ElementDeclaration _border_element = { .layout = {
		.sizing = { // Grow to fit whole parent.
			.width = CLAY_SIZING_GROW(0),
			.height = CLAY_SIZING_GROW(0) } },
		.floating = { // Attach to parent top left corner and allow pointer passthrough.
			.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
			.attachTo = CLAY_ATTACH_TO_PARENT }, 
		.border = { // White border, one pixel thick.
			.color = Color::WHITE,
			.width = { .left = 1, .right = 1, .top = 1, .bottom = 1 } }
	};

	void _layout_border() {
		const auto parent = Clay__GetParentElementId();
		const auto child = Clay__HashString(CLAY_STRING("border"), parent);
		CLAY(CLAY_ID_LOCAL("border"), _border_element) {}
	}

	void _layout_textbox_text(std::string_view text) {
		CLAY(CLAY_ID_LOCAL("textbox_text"), { .layout = {
			.sizing = { .height = CLAY_SIZING_FIXED(32.f) }, // Fixed height equal to image/portrait size.
			.padding = { .left = 2, .right = 1 } } }) // Extra padding to compensate for (visually) uneven bounding box.
		{
			shared::layout_text(text);
		}
	}

	void _layout_textbox_image(ImageData* image) {
		if (!image) return; // DEFENSIVE
		CLAY(CLAY_ID("textbox_image_container"), { .layout = {
			// Two extra pixels at top and bottom to align better with text.
			.sizing = {
				.width = CLAY_SIZING_FIXED(32.f),
				.height = CLAY_SIZING_FIXED(32.f) } },
			//.backgroundColor = Color::RED // for debugging
			}) {
			CLAY(CLAY_ID("textbox_image"), { .layout = {
				.sizing = { .width = (float)image->size.x, .height = (float)image->size.y },
				.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
				.image = { .imageData = image }
				}) {}
		}
	}

	void _layout_textbox(const Textbox& textbox) {
		const std::string_view text = get_typed_text();

		constexpr uint16_t SPACING = 5;
		constexpr float MIN_WIDTH = 16 * 16.f; // 16 tiles wide

		CLAY(CLAY_ID_LOCAL("textbox"), { .layout = {
			.sizing = { .width = CLAY_SIZING_FIT(MIN_WIDTH, 0.f) },
			.padding = CLAY_PADDING_ALL(SPACING + 1), // One extra pixel of padding to include border.
			.childGap = SPACING,
			.layoutDirection = CLAY_LEFT_TO_RIGHT },
			.backgroundColor = Color::BLACK })
		{
			_layout_border();
			if (textbox.image) {
				_layout_textbox_image(textbox.image);
			}
			_layout_textbox_text(text);
		}
	}

	void _layout_options() {
		CLAY(CLAY_ID_LOCAL("options"), { .layout = {} }) {}
	}

	void _layout_options(std::span<const TextboxOption> options) {
		constexpr uint16_t SPACING = 5;
		CLAY(CLAY_ID("options_box"), { .layout = {
			.padding = CLAY_PADDING_ALL(SPACING + 1), // One extra pixel of padding to include border.
			.layoutDirection = CLAY_LEFT_TO_RIGHT },
			.backgroundColor = Color::BLACK }) {
			_layout_border();
			CLAY(CLAY_ID_LOCAL("options"), { .layout = {
				.padding = { .left = 2, .right = 1 },
				.childGap = 3,
				.layoutDirection = CLAY_TOP_TO_BOTTOM } }) {
				for (size_t option = 0; option < options.size(); option++) {
					const TextboxOption& opt = options[option];
					CLAY(CLAY_SID_LOCAL(to_clay(opt.text))) {
						if (mouse_over()) {
							set_current_option(option);
						}
						Clay_TextElementConfig* text_config = &shared::default_text;
						if (option == get_current_option()) {
							text_config = &shared::hovered_text;
							if (mouse_over() && mouse_down()) {
								text_config = &shared::pressed_text;
							}
							if (mouse_over() && mouse_released()) {
								select_current_option();
							}
						}
						CLAY_TEXT(to_clay(opt.text), text_config);
					}
				}
			}
		}
	}

	void layout() {
		if (closed())
			return;

		CLAY(CLAY_ID("textboxes"), { .layout = {
			.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
			.padding = CLAY_PADDING_ALL(8),
			.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_BOTTOM },
			.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.floating = { .attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			const Textbox& textbox = get_current_textbox();
			CLAY(CLAY_ID_LOCAL("options_and_textbox"), { .layout = {
				.childGap = 8,
				.layoutDirection = CLAY_TOP_TO_BOTTOM } })
			{
				if (!is_typing_text() && !textbox.options.empty()) {
					_layout_options(textbox.options);
				}
				_layout_textbox(textbox);
			}
		}
	}
}
}