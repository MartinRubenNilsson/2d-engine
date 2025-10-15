#pragma once

namespace steam
{
	bool restart_app_if_necessary();
	bool is_steam_running();
	bool startup();
	void shutdown();
	void run_message_loop();
	bool is_overlay_active();

	std::string get_steam_id();
	std::vector<std::string> get_friends_persona_names();
}

