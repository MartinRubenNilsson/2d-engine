#pragma once

namespace window {
	struct Event;
}

namespace ui {
	enum class EventType {
		PlayGame,
		GoToMainMenu,
	};

	struct Event {
		EventType type;
	};

	namespace bindings {
		void on_click_play();
		void on_click_settings();
		void on_click_credits();
		void on_click_back();
		void on_click_resume();
		void on_click_main_menu();
	}

	bool get_next_event(Event& ev);
	bool is_menu_or_visible();
}
