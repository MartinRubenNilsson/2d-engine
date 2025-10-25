#include "stdafx.h"
#include "ui_shared.h"
#include "ui_data.h"
#include "ui.h"
#include "text_fonts.h"
#include "audio.h"

namespace ui {
namespace shared {

	text::FontId _font{};
	TextData _default_text_data{};

	Clay_TextElementConfig default_text{};
	Clay_TextElementConfig hovered_text{};
	Clay_TextElementConfig pressed_text{};

	Clay_ElementDeclaration menu_element{};
	Clay_ElementDeclaration main_menu_element{};
	Clay_ElementDeclaration menu_with_gray_bg_element{};

	void startup() {
		_font = text::load_font("assets/fonts/Mozart NBP.ttf");

		_default_text_data.shadow_offset = { 0.5f, 0.5f };
		//_default_text_data.shadow_color = Color::DIM_GRAY;

		default_text.userData = &_default_text_data;
		default_text.fontId = _font.id;
		default_text.fontSize = 13; // PITFALL: Not a power of 2 - not a problem, but maybe unexpected since it is a pixel font.
		default_text.lineHeight = 11;

		hovered_text = default_text;
		hovered_text.textColor = Color::SILVER;

		pressed_text = default_text;
		pressed_text.textColor = Color::DIM_GRAY;

		menu_element.layout.sizing.width = CLAY_SIZING_GROW(0);
		menu_element.layout.sizing.height = CLAY_SIZING_GROW(0);
		menu_element.layout.childGap = 6;
		menu_element.layout.childAlignment.x = CLAY_ALIGN_X_CENTER;
		menu_element.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER;
		menu_element.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
		menu_element.floating.attachTo = CLAY_ATTACH_TO_ROOT;

		main_menu_element = menu_element;
		main_menu_element.layout.childGap = 3;

		menu_with_gray_bg_element = menu_element;
		menu_with_gray_bg_element.backgroundColor = Color(64, 64, 64, 76);
	}

	void layout_text(std::string_view text) {
		CLAY_TEXT(to_clay(text), &default_text);
	}

	void layout_text_button(std::string_view text, void(*on_click)()) {
		CLAY(CLAY_SID_LOCAL(to_clay(text))) {
			if (mouse_enter()) {
				audio::create_event("event:/ui/snd_button_hover");
			}
			Clay_TextElementConfig* text_config = &default_text;
			if (mouse_over()) {
				text_config = &hovered_text;
				if (mouse_pressed()) {
					text_config = &pressed_text;
				}
				if (mouse_up()) {
					if (on_click) {
						on_click();
					}
					audio::create_event("event:/ui/snd_button_click");
				}
			}
			CLAY_TEXT(to_clay(text), text_config);
		}
	}
}
}