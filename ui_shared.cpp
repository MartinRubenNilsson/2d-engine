#include "stdafx.h"
#include "ui_shared.h"
#include "ui_data.h"
#include "text_fonts.h"
#include "audio.h"

namespace ui {
namespace shared {

	text::FontId _font{};
	TextData _default_text_data{};

	Clay_TextElementConfig default_text{};
	Clay_TextElementConfig _default_menu_button_text{};
	Clay_TextElementConfig _hovered_menu_button_text{};

	Clay_ElementDeclaration menu_element{};
	Clay_ElementDeclaration menu_with_gray_bg_element{};

	void startup() {
		_font = text::load_font("assets/fonts/Mozart NBP.ttf");

		_default_text_data.shadow_offset = { 0.5f, 0.5f };
		//_default_text_data.shadow_color = Color::DIM_GRAY;

		default_text.userData = &_default_text_data;
		default_text.fontId = _font.id;
		default_text.fontSize = 13; // PITFALL: Not a power of 2 - not a problem, but maybe unexpected since it is a pixel font.
		default_text.lineHeight = 11;

		_default_menu_button_text = default_text;
		_default_menu_button_text.textAlignment = CLAY_TEXT_ALIGN_CENTER;

		_hovered_menu_button_text = _default_menu_button_text;
		_hovered_menu_button_text.textColor = Color::SILVER;

		menu_element.layout.sizing.width = CLAY_SIZING_GROW(0);
		menu_element.layout.sizing.height = CLAY_SIZING_GROW(0);
		menu_element.layout.childGap = 6;
		menu_element.layout.childAlignment.x = CLAY_ALIGN_X_CENTER;
		menu_element.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER;
		menu_element.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
		menu_element.floating.attachTo = CLAY_ATTACH_TO_ROOT;

		menu_with_gray_bg_element = menu_element;
		menu_with_gray_bg_element.backgroundColor = Color(64, 64, 64, 76);
	}

	Clay_String to_clay(std::string_view string) {
		return { false, (int32_t)string.size(), string.data() };
	}

	void _on_hover_menu_button(Clay_ElementId element_id, Clay_PointerData pointer_data, intptr_t user_data) {
		using OnClick = void(*)(); // to make it easier to cast
		const OnClick on_click = (OnClick)user_data;
		if (!on_click) return;
		if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
			audio::create_event("event:/ui/snd_button_click");
			on_click();
		}
	}

	void layout_text(std::string_view text) {
		CLAY_TEXT(to_clay(text), &default_text);
	}

	void layout_menu_button(std::string_view text, void(*on_click)()) {
		const Clay_ElementId id = CLAY_SID_LOCAL(to_clay(text));
		CLAY(id) {
			const bool hovering = Clay_Hovered();
			bool started_hovering = false;
			static Clay_ElementId last_hovered_id{};
			if (hovering && last_hovered_id.id != id.id) {
				started_hovering = true;
				last_hovered_id = id;
			}
			if (!hovering && last_hovered_id.id == id.id) {
				last_hovered_id = {};
			}
			if (started_hovering) {
				audio::create_event("event:/ui/snd_button_hover");
			}
			CLAY_TEXT(to_clay(text), Clay_Hovered() ? &_hovered_menu_button_text : &_default_menu_button_text);
			Clay_OnHover(_on_hover_menu_button, (uintptr_t)on_click);
		}
	}
}
}