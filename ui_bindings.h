#pragma once

namespace ui {
	namespace bindings {
		extern std::string textbox_text; // RML
		extern bool textbox_has_options;
		extern std::vector<std::string> textbox_options;
		extern size_t textbox_selected_option;
	}

	bool is_variable_dirty(const std::string& name);
	void dirty_all_variables();
	void create_bindings();
}
