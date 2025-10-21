#include "stdafx.h"
#include "ui_rmlui.h"
#include "ui_rmlui_system_interface.h"
#include "ui_rmlui_render_interface.h"
#include "ui_bindings.h"
#include "ui_menus.h"
#include "ui_textbox.h"
#include "console.h"
#include "audio.h"
#include "window_events.h"
#include "files.h"

#ifdef _DEBUG_UI
#include <RmlUi/Debugger.h>
#endif

namespace ui {
	// All documents share this event listener.
	struct CommonEventListener : Rml::EventListener {
		void ProcessEvent(Rml::Event& ev) override {
			switch (ev.GetId()) {
			case Rml::EventId::Mouseover:
			{
				if (ev.GetTargetElement()->IsClassSet("menu-button")) {
					audio::create_event({ .path = "event:/ui/snd_button_hover" });
				}
			} break;
			case Rml::EventId::Click:
			{
				if (ev.GetTargetElement()->IsClassSet("menu-button")) {
					audio::create_event({ .path = "event:/ui/snd_button_click" });
				}
			} break;
			}
		}
	};

	constexpr float _DT_ACCUMULATOR_MIN = 1.0f / 60.0f;
	float _dt_accumulator = 0.f;
	bool debug_rmlui = false;
	RmlUiSystemInterface _system_interface;
	RmlUiRenderInterface _render_interface;
	Rml::Context* _context = nullptr;
#ifdef _DEBUG_UI
	Rml::Context* _debugger_context = nullptr; // The debugger needs its own context to render at the right size.
#endif
	CommonEventListener _common_event_listener;
	std::vector<Event> _events;

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
			_events.push_back({ EventType::PlayGame });
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
			_events.push_back({ EventType::GoToMainMenu });
		}
	}

	void startup_rmlui() {
		Rml::SetSystemInterface(&_system_interface);
		Rml::SetRenderInterface(&_render_interface);
		Rml::Initialise();
		_context = Rml::CreateContext("main", Rml::Vector2i());
#ifdef _DEBUG_UI
		_debugger_context = Rml::CreateContext("debugger", Rml::Vector2i());
		Rml::Debugger::Initialise(_debugger_context);
		Rml::Debugger::SetContext(_context);
#endif
		create_bindings();
		create_textbox_presets();
	}

	void shutdown_rmlui() {
		Rml::Shutdown();
		_context = nullptr;
#ifdef _DEBUG_UI
		_debugger_context = nullptr;
#endif
	}

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

	double _mouse_position_x = 0;
	double _mouse_position_y = 0;
	bool _mouse_is_down = false;
	float _scroll_delta_x = 0.f;

	void handle_window_event_for_rmlui(const window::Event& ev) {
		switch (ev.type) {
		case window::EventType::FramebufferSize:
		{
			set_viewport(ev.size.width, ev.size.height);
			_context->SetDimensions(Rml::Vector2i(ev.size.width, ev.size.height));
#ifdef _DEBUG_UI
			_debugger_context->SetDimensions(Rml::Vector2i(ev.size.width, ev.size.height));
#endif
			float dp_ratio_x = (float)ev.size.width / GAME_FRAMEBUFFER_WIDTH;
			float dp_ratio_y = (float)ev.size.height / GAME_FRAMEBUFFER_HEIGHT;
			float dp_ratio = std::min(dp_ratio_x, dp_ratio_y);
			_context->SetDensityIndependentPixelRatio(dp_ratio);
			// Don't set density independent pixel ratio for the debugger context!
			// It should always be 1.0, so that it remains the same size.

		} break;
#if 1
		case window::EventType::KeyPress:
		{
			if (ev.key.code == window::Key::Escape) {
				_on_escape_key_pressed();
			}
			Rml::Input::KeyIdentifier key_identifier = _translate_key_identifier_to_rml(ev.key.code);
			int key_modifier_flags = _translate_key_modifier_flags_to_rml(ev.key.modifier_key_flags);
			_context->ProcessKeyDown(key_identifier, key_modifier_flags);
#ifdef _DEBUG_UI
			_debugger_context->ProcessKeyDown(key_identifier, key_modifier_flags);
#endif
		} break;
		case window::EventType::KeyRelease:
		{
			Rml::Input::KeyIdentifier key_identifier = _translate_key_identifier_to_rml(ev.key.code);
			int key_modifier_flags = _translate_key_modifier_flags_to_rml(ev.key.modifier_key_flags);
			_context->ProcessKeyUp(key_identifier, key_modifier_flags);
#ifdef _DEBUG_UI
			_debugger_context->ProcessKeyUp(key_identifier, key_modifier_flags);
#endif
		} break;
#endif
		case window::EventType::MouseMove:
		{
			// CRITICAL: We get big frame drops when calling ProcessMouseMove() a lot,
			// since it invokes a heavy update hover chain call. This happens when for example
			// when we move the mouse around a lot. I dropped from 350 FPS to 230 FPS!
			// Hence, let's only call ProcessMouseMove() when we're in a menu,
			// since it's only then that we're using the mouse position (to press buttons).
			int key_modifier_flags = _translate_key_modifier_flags_to_rml(ev.mouse_button.modifier_key_flags);
			if (debug_rmlui || get_top_menu() != MenuType::Count) {
				_context->ProcessMouseMove((int)ev.mouse_move.x, (int)ev.mouse_move.y, key_modifier_flags);
			}
#ifdef _DEBUG_UI
			if (debug_rmlui) {
				_debugger_context->ProcessMouseMove((int)ev.mouse_move.x, (int)ev.mouse_move.y, key_modifier_flags);
			}
#endif
			_mouse_position_x = (int)ev.mouse_move.x;
			_mouse_position_y = (int)ev.mouse_move.y;

		} break;
		case window::EventType::MouseButtonPress:
		{
			int key_modifier_flags = _translate_key_modifier_flags_to_rml(ev.mouse_button.modifier_key_flags);
			_context->ProcessMouseButtonDown((int)ev.mouse_button.button, key_modifier_flags);
#ifdef _DEBUG_UI
			_debugger_context->ProcessMouseButtonDown((int)ev.mouse_button.button, key_modifier_flags);
#endif
			_mouse_is_down = true;
		} break;
		case window::EventType::MouseButtonRelease:
		{
			int key_modifier_flags = _translate_key_modifier_flags_to_rml(ev.mouse_button.modifier_key_flags);
			_context->ProcessMouseButtonUp((int)ev.mouse_button.button, key_modifier_flags);
#ifdef _DEBUG_UI
			_debugger_context->ProcessMouseButtonUp((int)ev.mouse_button.button, key_modifier_flags);
#endif
			_mouse_is_down = false;
		} break;
#if 0
		case sf::Event::MouseWheelMoved:
		{
			_context->ProcessMouseWheel(float(-ev.mouseWheel.delta), key_modifier_flags);
			_debugger_context->ProcessMouseWheel(float(-ev.mouseWheel.delta), key_modifier_flags);
		} break;
		case sf::Event::MouseLeft:
		{
			_context->ProcessMouseLeave();
			_debugger_context->ProcessMouseLeave();
		} break;
		case sf::Event::TextEntered:
		{
			Rml::Outfit c = Rml::Outfit(ev.text.unicode);
			if (c == Rml::Outfit('\r')) {
				c = Rml::Outfit('\n');
			}
			if (ev.text.unicode >= 32 || c == Rml::Outfit('\n')) {
				_context->ProcessTextInput(c);
				_debugger_context->ProcessTextInput(c);
			}
		} break;
#endif
		}
	}

	void update_rmlui(float dt) {
		// As an optimization, update the UI at a lower FPS than the game.
		_dt_accumulator += dt;
		if (_dt_accumulator < _DT_ACCUMULATOR_MIN) return;

		update_textbox(_dt_accumulator);
		dirty_all_variables();
		_context->Update();
#ifdef _DEBUG_UI
		if (debug_rmlui != Rml::Debugger::IsVisible()) {
			Rml::Debugger::SetVisible(debug_rmlui);
		}
		_debugger_context->Update();
#endif

		_dt_accumulator = 0.0f;
	}

	void render_rmlui() {
		prepare_render_state();
		_context->Render();
#ifdef _DEBUG_UI
		_debugger_context->Render();
#endif
		restore_render_state();
	}

	void load_font_from_file(const std::string& path) {
		Rml::LoadFontFace(path);
	}

	void load_document_from_file(const std::string& path) {
		Rml::ElementDocument* doc = _context->LoadDocument(path);
		if (!doc) {
			console::log_error("Failed to load RmlUi document: " + path);
			return;
		}
		doc->SetId(files::get_stem(path));
	}

	void add_event_listeners() {
		// Add common event listener to all documents.
		for (int i = 0; i < _context->GetNumDocuments(); ++i) {
			Rml::ElementDocument* doc = _context->GetDocument(i);
			doc->AddEventListener(Rml::EventId::Mouseover, &_common_event_listener);
			doc->AddEventListener(Rml::EventId::Click, &_common_event_listener);
		}

		add_menu_event_listeners();
		add_textbox_event_listeners();
	}

#if 0
	// Reloads all documents' style sheets from <style> tags
	// and external style sheets, but not inline "style" attributes.
	void reload_styles() {
		for (int i = 0; i < _context->GetNumDocuments(); ++i) {
			_context->GetDocument(i)->ReloadStyleSheet();
		}
	}
#endif

	void show_document(const std::string& name) {
		if (Rml::ElementDocument* doc = _context->GetDocument(name)) {
			doc->Show();
		}
	}

	bool get_next_event(Event& ev) {
		if (_events.empty()) return false;
		ev = _events.back();
		_events.pop_back();
		return true;
	}

	bool is_menu_or_textbox_visible() {
		return (get_top_menu() != MenuType::Count) || is_textbox_open();
	}
}