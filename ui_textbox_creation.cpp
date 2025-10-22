#include "stdafx.h"
#include "ui_textbox.h"
#include "ui_data.h"
#include "graphics.h"
#include "map.h"

#include "ui_rmlui.h" // TODO: remove

namespace ui {
namespace textbox {
	std::string_view get_sprite_name(TextboxSprite sprite) { // TODO: remove
		switch (sprite) {
			case TextboxSprite::None:      return "";
			case TextboxSprite::Skull:     return "icon-skull";
			case TextboxSprite::GoldenKey: return "icon-golden-key";
			default:                       return "";
		}
	}

	constexpr std::string_view _ITEM_FANFARE_SOUND = "event:/snd_item_fanfare";

	Handle<graphics::Texture> _icons_texture{};
	ImageData _skull_image{};

	void _create_images() {
		_icons_texture = graphics::load_texture("assets/textures/ui/icons.png");

		constexpr Vec2u size = { 16, 16 };

		_skull_image.texture = _icons_texture;
		_skull_image.position = size * Vec2u(0, 9);
		_skull_image.size = size;
	}

	void _create_textboxes() {
		_create_images();

		create_textbox({
			.path = "player/die/0",
			.text = "You are <span style='color: red'>deader than dead</span>!<br/>Oh, what a pity that your adventure should end here, and so soon...",
			.sprite_DEPRECATED = TextboxSprite::Skull,
			.image = &_skull_image
			});
		create_textbox({
			.path = "player/die/1",
			.text = "Would you like to try again?",
			.sprite_DEPRECATED = TextboxSprite::Skull,
			.image = &_skull_image,
			.options = { "Yes", "No" },
			.on_option_selected = [](std::string_view option) {
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