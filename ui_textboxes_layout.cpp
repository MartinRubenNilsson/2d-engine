#include "stdafx.h"
#include "ui_textboxes.h"
#include "ui_shared.h"

namespace ui {
namespace textboxes {

	void _layout_textbox_border() {
		CLAY(CLAY_ID_LOCAL("textbox_border"), { .layout = {
			.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } }, // Grow to fit whole parent.
			.floating = { .attachTo = CLAY_ATTACH_TO_PARENT }, // Attach to parent top left corner.
			.border = { .color = Color::WHITE, .width = { 1, 1, 1, 1 } } }) {} // White border, one pixel thick.
	}

	void _layout_textbox_text(std::string_view text) {
		constexpr uint16_t PADDING = 4;
		CLAY(CLAY_ID_LOCAL("textbox_text"), { .layout = { .padding = { PADDING + 1, PADDING, PADDING, PADDING } } }) {
			shared::layout_text(text);
		}
	}

	void layout_textbox() {
		constexpr float MIN_WIDTH = 16 * 16.f; // 16 tiles wide
		constexpr float MIN_HEIGHT = 3 * 16.f; // 3 tiles tall
		CLAY(CLAY_ID_LOCAL("textbox"), { .layout = {
			.sizing = { .width = CLAY_SIZING_FIT(MIN_WIDTH, 0.f), .height = CLAY_SIZING_FIT(MIN_HEIGHT, 0.f) },
			.padding = CLAY_PADDING_ALL(1) },
			.backgroundColor = Color::BLACK })
		{
			_layout_textbox_border();
			_layout_textbox_text("Hello, world! This is Martin speaking\nindeed. I'm fine thank you.\nAnd anotha line for u.");
		}
	}

	void _layout_textbox(const Textbox& textbox) {
		// TODO
	}

	void layout() {
		CLAY(CLAY_ID("textboxes"), { .layout = {
			.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
			.padding = CLAY_PADDING_ALL(8),
			.childGap = 3,
			.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
			.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.floating = { .attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			layout_textbox();
		}
	}
}
}