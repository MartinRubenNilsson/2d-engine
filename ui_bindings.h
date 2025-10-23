#pragma once

namespace ui {
	bool is_variable_dirty(const std::string& name);
	void dirty_all_variables();
	void create_bindings();
}
