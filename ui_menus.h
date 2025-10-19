#pragma once

namespace ui {
	enum class MenuType {
		Main,
		Pause,
		Settings,
		Credits,
		Count,
	};

	void add_menu_event_listeners();

	// We use a stack to manage menus.
	// The top menu is the only one that is visible.

	MenuType get_top_menu(); // Returns MenuType::Count if stack is empty.
	void push_menu(MenuType type);
	void pop_menu();
	void pop_all_menus();
}

