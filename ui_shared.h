#pragma once
#include "ui_data.h"

// The stuff in this file is meant to be shared between multiple UI .cpp files.

namespace ui {
namespace shared {

	extern Clay_TextElementConfig default_text;
	extern Clay_TextElementConfig hovered_text;
	extern Clay_TextElementConfig pressed_text;
	extern Clay_ElementDeclaration menu_element;
	extern Clay_ElementDeclaration main_menu_element;
	extern Clay_ElementDeclaration menu_with_gray_bg_element;

	void startup();

	void layout_text(std::string_view text);
	void layout_text_button(std::string_view text, void(*on_click)());
}
}