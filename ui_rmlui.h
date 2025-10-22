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

	void startup_rmlui();
	void shutdown_rmlui();
	void handle_window_event_for_rmlui(const window::Event& ev);
	void update_rmlui(float dt);
	void render_rmlui();

	void load_font_from_file(const std::string& path);
	void load_document_from_file(const std::string& path);
	void add_event_listeners();

	void show_document(const std::string& name);

	bool get_next_event(Event& ev);
	bool is_menu_or_visible();
}
