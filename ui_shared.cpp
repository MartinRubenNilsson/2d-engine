#include "stdafx.h"
#include "ui_shared.h"
#include "ui_types.h"
#include "text_fonts.h"

namespace ui {
namespace shared {

	text::FontId _font{};
	TextData _text_data{};

	Clay_TextElementConfig text_config{};

	void startup() {
		_font = text::load_font("assets/fonts/Mozart NBP.ttf");
		_text_data.shadow_offset = { 0.5f, 0.5f };
		text_config.fontId = _font.id;
		text_config.fontSize = 13; // PITFALL: Not a power of 2 - not a problem, but maybe unexpected since it is a pixel font.
		text_config.userData = &_text_data;
	}

} // namespace shared
} // namespace ui