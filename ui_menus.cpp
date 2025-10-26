#include "stdafx.h"
#include "ui_menus.h"
#include "settings.h"
#include "ui_main_menu.h"
#include "ui_pause_menu.h"
#include "ui_credits_menu.h"
#include "ui_settings_menu.h"

namespace ui {
#if 0
	struct SettingsMenuEventListener : public Rml::EventListener {
		void ProcessEvent(Rml::Event& ev) override {
			switch (ev.GetId()) {
				case Rml::EventId::Show: {
					if (Rml::ElementDocument* doc = _get_menu_document(MenuType::Settings)) {
						if (Rml::Element* el = doc->GetElementById("checkbox-fullscreen"))
							settings::app_settings.fullscreen ? el->SetAttribute("checked", "true") : el->RemoveAttribute("checked");
						if (Rml::Element* el = doc->GetElementById("select-window-scale"))
							el->SetAttribute("value", std::to_string(settings::app_settings.window_scale));
						if (Rml::Element* el = doc->GetElementById("checkbox-vsync"))
							settings::app_settings.vsync ? el->SetAttribute("checked", "true") : el->RemoveAttribute("checked");
						if (Rml::Element* el = doc->GetElementById("range-volume-master"))
							el->SetAttribute("value", std::to_string(settings::app_settings.volume_master));
						if (Rml::Element* el = doc->GetElementById("range-volume-sound"))
							el->SetAttribute("value", std::to_string(settings::app_settings.volume_sound));
						if (Rml::Element* el = doc->GetElementById("range-volume-music"))
							el->SetAttribute("value", std::to_string(settings::app_settings.volume_music));
					}
				} break;
				case Rml::EventId::Submit: {
					std::string type = ev.GetType();
					settings::app_settings.fullscreen = (ev.GetParameter("fullscreen", Rml::String("off")) == "on");
					settings::app_settings.window_scale = ev.GetParameter("window-scale", 5);
					settings::app_settings.vsync = (ev.GetParameter("vsync", Rml::String("off")) == "on");
					settings::app_settings.volume_master = ev.GetParameter("volume-master", 1.f);
					settings::app_settings.volume_sound = ev.GetParameter("volume-sound", 1.f);
					settings::app_settings.volume_music = ev.GetParameter("volume-music", 1.f);
					settings::apply(settings::app_settings);
					settings::save_to_file(settings::APP_SETTINGS_PATH, settings::app_settings);
				} break;
			}
		}
	};
#endif

#if 0
	void RmlUiSystemInterface::SetMouseCursor(const Rml::String& cursor_name) {
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
#endif
}