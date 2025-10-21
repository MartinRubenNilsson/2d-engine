#include "stdafx.h"
#include "ui_shared.h"
#include "ui_types.h"
#include "text_fonts.h"
#include "audio.h"
#include "console.h"

namespace ui {
namespace shared {

	text::FontId _font{};
	TextData _text_data{};

	Clay_TextElementConfig text_config{};
	Clay_TextElementConfig _normal_menu_button_text_config{};
	Clay_TextElementConfig _hovered_menu_button_text_config{};

	void startup() {
		_font = text::load_font("assets/fonts/Mozart NBP.ttf");

		_text_data.shadow_offset = { 0.5f, 0.5f };

		text_config.fontId = _font.id;
		text_config.fontSize = 13; // PITFALL: Not a power of 2 - not a problem, but maybe unexpected since it is a pixel font.
		text_config.userData = &_text_data;

		_normal_menu_button_text_config = text_config;
		_normal_menu_button_text_config.textAlignment = CLAY_TEXT_ALIGN_CENTER;

		_hovered_menu_button_text_config = _normal_menu_button_text_config;
		_hovered_menu_button_text_config.textColor = Color::SILVER;
	}

	Clay_String to_clay(std::string_view string) {
		return { false, (int32_t)string.size(), string.data() };
	}

	void _on_hover_menu_button(Clay_ElementId element_id, Clay_PointerData pointer_data, intptr_t user_data) {
		using OnClick = void(*)(); // to make it easier to cast
		const OnClick on_click = (OnClick)user_data;
		if (!on_click) return;
		if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
			audio::create_event({ .path = "event:/ui/snd_button_click" });
			on_click();
		}
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
				audio::create_event({ .path = "event:/ui/snd_button_hover" });
			}
			CLAY_TEXT(to_clay(text), Clay_Hovered() ? &_hovered_menu_button_text_config : &_normal_menu_button_text_config);
			Clay_OnHover(_on_hover_menu_button, (uintptr_t)on_click);
		}
	}

} // namespace shared
} // namespace ui