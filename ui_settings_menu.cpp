#include "stdafx.h"
#include "ui_settings_menu.h"
#include "ui_main_menu.h"
#include "ui_pause_menu.h"
#include "ui_shared.h"
#include "map.h"
#include "input.h"

namespace ui {
namespace settings_menu {
	bool show = false;

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

	void _on_select_back() {
		show = false;
		if (map::is_open()) {
			pause_menu::show = true;
		} else {
			main_menu::show = true;
		}
	}

	void update() {
		if (input::pressed(window::Key::Escape)) {
			_on_select_back();
		}
	}

	void layout() {
		CLAY(CLAY_ID("settings_menu"), shared::menu_with_gray_bg_element) {
			shared::layout_text_button("Back", _on_select_back);
		}
	}
}
}