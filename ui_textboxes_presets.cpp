#include "stdafx.h"
#include "ui_textboxes.h"
#include "ui_data.h"
#include "graphics.h"
#include "map.h"

#include "ui_rmlui.h" // TODO: remove

namespace ui {
namespace textboxes {
	constexpr std::string_view _ITEM_FANFARE_SOUND = "event:/snd_item_fanfare";

	Handle<graphics::Texture> _icons_texture{};
	ImageData _skull_image{};

	void _startup_images() {
		_icons_texture = graphics::load_texture("assets/textures/ui/icons.png");

		constexpr Vec2u size = { 32, 32 };

		_skull_image.texture = _icons_texture;
		_skull_image.position = size * Vec2u(0, 9);
		_skull_image.size = size;
	}

	void _startup_presets() {
		_startup_images();

		add_preset({
			.path = "player/die/0",
			.text = "You are deader than dead! Oh, what a pity that your adventure should end here, and so soon...",
			.image = &_skull_image
			});
		add_preset({
			.path = "player/die/1",
			.text = "Would you like to try again?",
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