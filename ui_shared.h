#pragma once
#include "ui_data.h"

// The stuff in this file is meant to be shared between multiple UI .cpp files.

namespace ui {
namespace shared {

	extern Clay_TextElementConfig default_text;
	extern Clay_ElementDeclaration menu_element;
	extern Clay_ElementDeclaration menu_with_gray_bg_element;

	void startup();

	Clay_String to_clay(std::string_view string);

	void layout_menu_button(std::string_view text, void(*on_click)());

}
}