#include "stdafx.h"
#include "ui_textboxes.h"
#include "ui_data.h"
#include "ui_main_menu.h"
#include "graphics.h"
#include "map.h"

namespace ui {
namespace textboxes {
	constexpr std::string_view _ITEM_FANFARE_SOUND = "event:/snd_item_fanfare";

	Handle<graphics::Texture> _icons_texture{};
	ImageData _skull_image{};

	void _startup_images() {
		_icons_texture = graphics::load_texture("assets/textures/ui/icons.png");

		constexpr Vec2u icon_size = { 32, 32 };

		_skull_image.texture = _icons_texture;
		_skull_image.position = icon_size * Vec2u(0, 9);
		_skull_image.size = icon_size;
	}

	void _go_back_to_main_menu() {
		map::close([] {
			ui::main_menu::show = true;
			});
	}

	void _add_textboxes() {
		_startup_images();

		add_textbox({
			.path = "player/die/0",
			.text = "You are deader than dead! Oh, what a pity that your adventure should end here, and so soon...",
			.image = &_skull_image
			});
		add_textbox({
			.path = "player/die/1",
			.text = "Would you like to try again?",
			.image = &_skull_image,
			.options = {
				{ "Yes", [] { map::reset(); } },
				{ "No", _go_back_to_main_menu } }
			});

		add_textbox({
			.path = "forest/stonehenge/billboard",
			.text = "You are a doofus. A total failure.",
			});
	}
}
}