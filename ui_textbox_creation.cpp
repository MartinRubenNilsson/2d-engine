#include "stdafx.h"
#include "ui_textbox.h"
#include "map.h"

#include "ui_rmlui.h" // TODO: remove

namespace ui {
namespace textbox {
	std::string_view get_sprite_name(TextboxSprite sprite) {
		switch (sprite) {
			case TextboxSprite::None:      return "";
			case TextboxSprite::Skull:     return "icon-skull";
			case TextboxSprite::GoldenKey: return "icon-golden-key";
			default:                       return "";
		}
	}

	const std::string_view DEFAULT_TYPING_SOUND = "event:/snd_txt1";
	const std::string_view OPENING_SOUND_ITEM_FANFARE = "event:/snd_item_fanfare";

	void _create_textboxes() {
		create_textbox({
			.path = "player/die/0",
			.text = "You are <span style='color: red'>deader than dead</span>!<br/>Oh, what a pity that your adventure should end here, and so soon...",
			.sprite = TextboxSprite::Skull
			});
		create_textbox({
			.path = "player/die/1",
			.text = "Would you like to try again?",
			.sprite = TextboxSprite::Skull,
			.options = { "Yes", "No" },
			.options_callback = [](std::string_view option) {
				if (option == "Yes") {
					map::reset();
				} else if (option == "No") {
					bindings::on_click_main_menu();
				}
			}
			});
	}
}
}