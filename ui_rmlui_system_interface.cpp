#include "stdafx.h"
#include "ui_rmlui_system_interface.h"
#include "window.h"

namespace ui
{
	double RmlUiSystemInterface::GetElapsedTime() {
		return window::get_elapsed_time();
	}

	void RmlUiSystemInterface::SetMouseCursor(const Rml::String& cursor_name)
	{
		if (cursor_name.empty() || cursor_name == "arrow") {
			//window::set_cursor_shape(window::CursorShape::Arrow);
			window::set_cursor_shape(window::CursorShape::HandPoint);
		} else if (cursor_name == "move") {
			//window::set_cursor_shape(sf::Cursor::SizeAll);
		} else if (cursor_name == "pointer") {
			window::set_cursor_shape(window::CursorShape::HandPointUp);
		} else if (cursor_name == "resize") {
			//window::set_cursor_shape(sf::Cursor::SizeTopLeftBottomRight);
		} else if (cursor_name == "cross") {
			window::set_cursor_shape(window::CursorShape::Crosshair);
		} else if (cursor_name == "text") {
			window::set_cursor_shape(window::CursorShape::Quill);
		} else if (cursor_name == "unavailable") {
			//window::set_cursor_shape(sf::Cursor::NotAllowed);
		} else if (cursor_name.starts_with("rmlui-scroll")) {
			//window::set_cursor_shape(sf::Cursor::SizeAll);
		}
	}

	void RmlUiSystemInterface::SetClipboardText(const Rml::String& text) {
		//window::set_clipboard_string({ text.begin(), text.end() });
	}

	void RmlUiSystemInterface::GetClipboardText(Rml::String& text) {
		//std::u8string_view string = window::get_clipboard_string();
		//text.assign()
	}
}
