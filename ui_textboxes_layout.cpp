#include "stdafx.h"
#include "ui_textboxes.h"
#include "ui_shared.h"

namespace ui {
namespace textboxes {

	void _layout_textbox_border() {
		CLAY(CLAY_ID_LOCAL("textbox_border"), { .layout = {
			.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } }, // Grow to fit whole parent.
			.floating = { .attachTo = CLAY_ATTACH_TO_PARENT }, // Attach to parent top left corner.
			.border = { .color = Color::WHITE, .width = { .left = 1, .right = 1, .top = 1, .bottom = 1 } } }) {} // White border, one pixel thick.
	}

	void _layout_textbox_text(std::string_view text) {
		CLAY(CLAY_ID_LOCAL("textbox_text"), { .layout = {
			.sizing = { .height = CLAY_SIZING_FIXED(34.f) }, // Fixed height equal to image/portrait size plus padding.
			.padding = { .left = 2, .right = 1, .bottom = 2 } } }) // Extra padding to compensate for (visually) uneven bounding box.
		{
			shared::layout_text(text);
		}
	}

	void _layout_textbox_image(ImageData* image) {
		if (!image) return; // DEFENSIVE
		CLAY(CLAY_ID("textbox_image_container"), { .layout = {
			.sizing = {
				.width = CLAY_SIZING_FIXED(34.f), // Two extra pixels at top and bottom to align better with text
				.height = CLAY_SIZING_FIXED(34.f) } },
				//.backgroundColor = Color::RED // for debugging
			}) {
			CLAY(CLAY_ID("textbox_image"), { .layout = {
				.sizing = {
					.width = (float)image->size.x,
					.height = (float)image->size.y },
					.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
					.image = { .imageData = image }
				}) {}
		}
	}

	extern ImageData _skull_image;

	void _layout_textbox() {
		const Textbox* textbox = get_current_textbox();
		if (!textbox) return;

		constexpr uint16_t SPACING = 5;
		constexpr float MIN_WIDTH = 16 * 16.f; // 16 tiles wide
		CLAY(CLAY_ID_LOCAL("textbox"), { .layout = {
			.sizing = { .width = CLAY_SIZING_FIT(MIN_WIDTH, 0.f) },
			.padding = CLAY_PADDING_ALL(SPACING + 1), // One extra pixel of padding to include border.
			.childGap = SPACING,
			.layoutDirection = CLAY_LEFT_TO_RIGHT },
			.backgroundColor = Color::BLACK })
		{
			_layout_textbox_border();
			if (textbox->image) {
				_layout_textbox_image(textbox->image);
			}
			_layout_textbox_text(get_current_typed_text()); // TODO
		}
	}

	void layout() {
		CLAY(CLAY_ID("textboxes"), { .layout = {
			.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
			.padding = CLAY_PADDING_ALL(8),
			//.childGap = 3,
			.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_BOTTOM },
			.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.floating = { .attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			_layout_textbox();
		}
	}
}
}