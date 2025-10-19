#include "stdafx.h"
#include "ui_hud.h"
#include "ui_types.h"
#include "graphics.h"
#include "text_fonts.h"

namespace ui {
namespace hud {
	bool show = false;
	unsigned int max_health = 3;
	unsigned int health = 2;
	unsigned int rupees = 0;
	unsigned int arrows = 0;
	unsigned int bombs = 0;

	text::FontId _font{};
	TextData _text_data{ .shadow_offset = { 0.5f, 0.5f } };
	Handle<graphics::Texture> _spritesheet{};
	ImageData _filled_heart_image{};
	ImageData _empty_heart_image{};

	void startup() {
		_font = text::load_font("assets/fonts/Mozart NBP.ttf");
		_spritesheet = graphics::load_texture("assets/ui/hud.png");
		_filled_heart_image.texture = _spritesheet;
		_filled_heart_image.rect_position = { 16, 16 };
		_filled_heart_image.rect_size = { 16, 16 };
		_empty_heart_image.texture = _spritesheet;
		_empty_heart_image.rect_position = { 16 * 4, 16 };
		_empty_heart_image.rect_size = { 16, 16 };
	}

	std::string _rupees_string;
	std::string _arrows_string;
	std::string _bombs_string;

	void update(float dt) {
		_rupees_string = "Rupees: " + std::to_string(rupees);
		_arrows_string = "Arrows: " + std::to_string(arrows);
		_bombs_string  = "Bombs:  " + std::to_string(bombs);
	}

	void _layout_heart(unsigned int index, bool filled) {
		ImageData* heart_image = filled ? &_filled_heart_image : &_empty_heart_image;
		CLAY(CLAY_IDI("HudHeart", index), { .layout = {
			.sizing = {
				.width = CLAY_SIZING_FIXED(16),
				.height = CLAY_SIZING_FIXED(16) } },
			.image = {
				.imageData = heart_image } })
		{}
	}

	void _layout_health() {
		CLAY(CLAY_ID("HudHealth"), { .layout = {
			.childGap = 3 } })
		{
			for (unsigned int i = 0; i < max_health; ++i) {
				_layout_heart(i, i < health);
			}
		}
	}

	Clay_String _to_clay_string(std::string_view string) {
		return { false, (int32_t)string.size(), string.data() };
	}

	void _layout_text(std::string_view string) {
		CLAY_TEXT(_to_clay_string(string), CLAY_TEXT_CONFIG({ .userData = &_text_data, .fontId = _font.id, .fontSize = 13 }));
	}

	void layout() {
		CLAY(CLAY_ID("HudContainer"), { .layout = {
			.padding = CLAY_PADDING_ALL(3),
			.childGap = 3,
			.layoutDirection = CLAY_TOP_TO_BOTTOM } })
		{
			_layout_health();
			_layout_text(_rupees_string);
			_layout_text(_arrows_string);
			_layout_text(_bombs_string);
		}
	}
}
}