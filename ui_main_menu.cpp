#include "stdafx.h"
#include "ui_main_menu.h"
#include "ui_settings_menu.h"
#include "ui_credits_menu.h"
#include "ui_shared.h"
#include "ui_hud.h"
#include "map.h"
#include "graphics.h"
#include "window.h"

namespace ui {
namespace main_menu {
	bool show = false;

	ImageData _game_logo_image{};

	void startup() {
		_game_logo_image.texture = graphics::load_texture("assets/textures/ui/game_logo.png");
		_game_logo_image.size = graphics::get_texture_size(_game_logo_image.texture);
	}

	float _hover_angle = 0.f; // in radians

	void update(float dt) {
		constexpr float ANGULAR_SPEED = 1.5f;
		_hover_angle += ANGULAR_SPEED * dt;
		_hover_angle = fmod(_hover_angle, M_2PI);
	}

	void _layout_game_logo_image() {
		CLAY(CLAY_ID("game_logo_image"), { .layout = {
			.sizing = {
				.width = (float)_game_logo_image.size.x,
				.height = (float)_game_logo_image.size.y } },
			.image = { .imageData = &_game_logo_image } } ) {
		}
	}

	void _layout_game_logo() {
		constexpr uint16_t AMPLITUDE = 3;
		const uint16_t padding_top = (uint16_t)(AMPLITUDE * (1.f - sin(_hover_angle)));
		const uint16_t padding_bottom = 2 * AMPLITUDE - padding_top;
		CLAY(CLAY_ID("game_logo"), { .layout = {
			.padding = {
				.top = (uint16_t)padding_top,
				.bottom = (uint16_t)padding_bottom } } })
		{
			_layout_game_logo_image();
		}
	}

	void _on_select_play() {
		map::open("summer_forest_00", [] {
			show = false;
			hud::show = true;
			});
	}

	void _on_select_settings() {
		show = false;
		settings_menu::show = true;
	}

	void _on_select_credits() {
		show = false;
		credits_menu::show = true;
	}

	void _on_select_quit() {
		window::set_should_close(true);
	}

	void layout() {
		CLAY(CLAY_ID("main_menu"), shared::main_menu_element) {
			_layout_game_logo();
			shared::layout_text_button("Play", _on_select_play);
			shared::layout_text_button("Settings", _on_select_settings);
			shared::layout_text_button("Credits", _on_select_credits);
			shared::layout_text_button("Quit", _on_select_quit);
		}
	}
}
}