#pragma once
#include "ui_types.h"

// The stuff in this file is meant to be shared between multiple UI .cpp files.

namespace ui {
namespace shared {

	extern Clay_TextElementConfig text_config;

	void startup();

	Clay_String to_clay(std::string_view string);

	void layout_menu_button(std::string_view text, void(*on_click)());

} // namespace shared
} // namespace ui