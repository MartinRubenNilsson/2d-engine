#include "stdafx.h"
#include "ui_rmlui.h"
#include "ui_menus.h"
#include "ui_textboxes.h"
#include "map.h"

namespace ui {
	void _on_escape_key_pressed() {
		MenuType current_menu = get_top_menu();
		if (current_menu == MenuType::Count) // no menus are open
			push_menu(MenuType::Pause);
		else if (current_menu != MenuType::Main) // don't pop main menu
			pop_menu();
	}

	namespace bindings {
		void on_click_play() {
			pop_all_menus();
			map::open("summer_forest_00");
		}

		void on_click_settings() {
			push_menu(MenuType::Settings);
		}

		void on_click_credits() {
			push_menu(MenuType::Credits);
		}

		void on_click_back() {
			pop_menu();
		}

		void on_click_resume() {
			pop_menu();
		}

		void on_click_main_menu() {
			pop_all_menus();
			push_menu(MenuType::Main);
			map::close(0.f);
		}
	}

#if 0
	Rml::Input::KeyIdentifier _translate_key_identifier_to_rml(window::Key key) {
		switch (key) {
		case window::Key::A:         return Rml::Input::KI_A;
		case window::Key::B:         return Rml::Input::KI_B;
		case window::Key::C:         return Rml::Input::KI_C;
		case window::Key::D:         return Rml::Input::KI_D;
		case window::Key::E:         return Rml::Input::KI_E;
		case window::Key::F:         return Rml::Input::KI_F;
		case window::Key::G:         return Rml::Input::KI_G;
		case window::Key::H:         return Rml::Input::KI_H;
		case window::Key::I:         return Rml::Input::KI_I;
		case window::Key::J:         return Rml::Input::KI_J;
		case window::Key::K:         return Rml::Input::KI_K;
		case window::Key::L:         return Rml::Input::KI_L;
		case window::Key::M:         return Rml::Input::KI_M;
		case window::Key::N:         return Rml::Input::KI_N;
		case window::Key::O:         return Rml::Input::KI_O;
		case window::Key::P:         return Rml::Input::KI_P;
		case window::Key::Q:         return Rml::Input::KI_Q;
		case window::Key::R:         return Rml::Input::KI_R;
		case window::Key::S:         return Rml::Input::KI_S;
		case window::Key::T:         return Rml::Input::KI_T;
		case window::Key::U:         return Rml::Input::KI_U;
		case window::Key::V:         return Rml::Input::KI_V;
		case window::Key::W:         return Rml::Input::KI_W;
		case window::Key::X:         return Rml::Input::KI_X;
		case window::Key::Y:         return Rml::Input::KI_Y;
		case window::Key::Z:         return Rml::Input::KI_Z;
		case window::Key::Num0:      return Rml::Input::KI_0;
		case window::Key::Num1:      return Rml::Input::KI_1;
		case window::Key::Num2:      return Rml::Input::KI_2;
		case window::Key::Num3:      return Rml::Input::KI_3;
		case window::Key::Num4:      return Rml::Input::KI_4;
		case window::Key::Num5:      return Rml::Input::KI_5;
		case window::Key::Num6:      return Rml::Input::KI_6;
		case window::Key::Num7:      return Rml::Input::KI_7;
		case window::Key::Num8:      return Rml::Input::KI_8;
		case window::Key::Num9:      return Rml::Input::KI_9;
		case window::Key::Numpad0:   return Rml::Input::KI_NUMPAD0;
		case window::Key::Numpad1:   return Rml::Input::KI_NUMPAD1;
		case window::Key::Numpad2:   return Rml::Input::KI_NUMPAD2;
		case window::Key::Numpad3:   return Rml::Input::KI_NUMPAD3;
		case window::Key::Numpad4:   return Rml::Input::KI_NUMPAD4;
		case window::Key::Numpad5:   return Rml::Input::KI_NUMPAD5;
		case window::Key::Numpad6:   return Rml::Input::KI_NUMPAD6;
		case window::Key::Numpad7:   return Rml::Input::KI_NUMPAD7;
		case window::Key::Numpad8:   return Rml::Input::KI_NUMPAD8;
		case window::Key::Numpad9:   return Rml::Input::KI_NUMPAD9;
		case window::Key::Left:      return Rml::Input::KI_LEFT;
		case window::Key::Right:     return Rml::Input::KI_RIGHT;
		case window::Key::Up:        return Rml::Input::KI_UP;
		case window::Key::Down:      return Rml::Input::KI_DOWN;
			//case window::Key::Add:       return Rml::Input::KI_ADD;
		case window::Key::Backspace: return Rml::Input::KI_BACK;
		case window::Key::Delete:    return Rml::Input::KI_DELETE;
			//case window::Key::Divide:    return Rml::Input::KI_DIVIDE;
		case window::Key::End:       return Rml::Input::KI_END;
		case window::Key::Escape:    return Rml::Input::KI_ESCAPE;
		case window::Key::F1:        return Rml::Input::KI_F1;
		case window::Key::F2:        return Rml::Input::KI_F2;
		case window::Key::F3:        return Rml::Input::KI_F3;
		case window::Key::F4:        return Rml::Input::KI_F4;
		case window::Key::F5:        return Rml::Input::KI_F5;
		case window::Key::F6:        return Rml::Input::KI_F6;
		case window::Key::F7:        return Rml::Input::KI_F7;
		case window::Key::F8:        return Rml::Input::KI_F8;
		case window::Key::F9:        return Rml::Input::KI_F9;
		case window::Key::F10:       return Rml::Input::KI_F10;
		case window::Key::F11:       return Rml::Input::KI_F11;
		case window::Key::F12:       return Rml::Input::KI_F12;
		case window::Key::F13:       return Rml::Input::KI_F13;
		case window::Key::F14:       return Rml::Input::KI_F14;
		case window::Key::F15:       return Rml::Input::KI_F15;
		case window::Key::Home:      return Rml::Input::KI_HOME;
		case window::Key::Insert:    return Rml::Input::KI_INSERT;
		case window::Key::LControl:  return Rml::Input::KI_LCONTROL;
		case window::Key::LShift:    return Rml::Input::KI_LSHIFT;
			//case window::Key::Multiply:  return Rml::Input::KI_MULTIPLY;
		case window::Key::Pause:     return Rml::Input::KI_PAUSE;
		case window::Key::RControl:  return Rml::Input::KI_RCONTROL;
			//case window::Key::Return:    return Rml::Input::KI_RETURN;
		case window::Key::RShift:    return Rml::Input::KI_RSHIFT;
		case window::Key::Space:     return Rml::Input::KI_SPACE;
			//case window::Key::Subtract:  return Rml::Input::KI_SUBTRACT;
		case window::Key::Tab:       return Rml::Input::KI_TAB;
		default:                     return Rml::Input::KI_UNKNOWN;
		}
	}

	int _translate_key_modifier_flags_to_rml(int key_modifier_flags) {
		int rml_key_modifiers = 0;
		if (key_modifier_flags & window::MODIFIER_KEY_CONTROL)   rml_key_modifiers |= Rml::Input::KM_CTRL;
		if (key_modifier_flags & window::MODIFIER_KEY_SHIFT)     rml_key_modifiers |= Rml::Input::KM_SHIFT;
		if (key_modifier_flags & window::MODIFIER_KEY_ALT)       rml_key_modifiers |= Rml::Input::KM_ALT;
		//if (key_modifier_flags & window::MODIFIER_KEY_SUPER)    rml_key_modifiers |= Rml::Input::KM_META; // Not sure if this is correct
		if (key_modifier_flags & window::MODIFIER_KEY_CAPS_LOCK) rml_key_modifiers |= Rml::Input::KM_CAPSLOCK;
		if (key_modifier_flags & window::MODIFIER_KEY_NUM_LOCK)  rml_key_modifiers |= Rml::Input::KM_NUMLOCK;
		//if (key_modifier_flags & window::MODIFIER_KEY_SCROLL_LOCK) rml_key_modifiers |= Rml::Input::KM_SCROLLLOCK; // We don't have KEY_SCROLL_LOCK
		return rml_key_modifiers;
	}
#endif

	bool is_menu_or_visible() {
		return (get_top_menu() != MenuType::Count) || !textboxes::closed();
	}
}