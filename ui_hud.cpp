#include "stdafx.h"
#include "ui_hud.h"
#include "ui_data.h"
#include "ui_shared.h"
#include "graphics.h"

namespace ui {
namespace hud {
	bool show = false;
	unsigned int max_health = 3;
	unsigned int health = 2;
	unsigned int rupees = 0;
	unsigned int arrows = 0;
	unsigned int bombs = 0;

	Handle<graphics::Texture> _spritesheet{};
	ImageData _filled_heart_image{};
	ImageData _empty_heart_image{};

	void startup() {
		_spritesheet = graphics::load_texture("assets/textures/ui/hud.png");

		_filled_heart_image.texture = _spritesheet;
		_filled_heart_image.position = { 16, 16 };
		_filled_heart_image.size = { 16, 16 };

		_empty_heart_image.texture = _spritesheet;
		_empty_heart_image.position = { 4 * 16, 16 };
		_empty_heart_image.size = { 16, 16 };
	}

	std::string _rupees_string;
	std::string _arrows_string;
	std::string _bombs_string;

	void update() {
		_rupees_string = "Rupees: " + std::to_string(rupees);
		_arrows_string = "Arrows: " + std::to_string(arrows);
		_bombs_string  = "Bombs:  " + std::to_string(bombs);
	}

	void _layout_heart(unsigned int index, bool filled) {
		ImageData& heart_image = filled ? _filled_heart_image : _empty_heart_image;
		CLAY(CLAY_IDI("hud_heart", index), { .layout = {
			.sizing = {
				.width = (float)heart_image.size.x,
				.height = (float)heart_image.size.y } },
			.image = {
				.imageData = &heart_image } })
		{}
	}

	void _layout_health() {
		CLAY(CLAY_ID("hud_health"), { .layout = { .childGap = 3 } }) {
			for (unsigned int i = 0; i < max_health; ++i) {
				_layout_heart(i, i < health);
			}
		}
	}

	void layout() {
		CLAY(CLAY_ID("hud"), { .layout = {
			.padding = CLAY_PADDING_ALL(3),
			.childGap = 2,
			.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.floating = { .attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			_layout_health();
			shared::layout_text(_rupees_string);
			shared::layout_text(_arrows_string);
			shared::layout_text(_bombs_string);
		}
	}
}
}