#pragma once

namespace ui
{
	namespace bindings
	{
		// HUD

		extern int hud_player_health;
		extern int hud_arrow_ammo;
		extern int hud_bomb_ammo;
		extern int hud_rupee_amount;

		// TEXTBOX

		extern std::string textbox_text; // RML
		extern bool textbox_has_sprite;
		extern std::string textbox_sprite;
		extern bool textbox_has_options;
		extern std::vector<std::string> textbox_options;
		extern size_t textbox_selected_option;
	}

	bool is_variable_dirty(const std::string& name);
	void dirty_all_variables();
	void create_bindings();
}

