#pragma once

namespace ecs {
	void setup_players(MapId map);
	void patch_players(const struct Patch& patch);
	void update_players(float dt);
	void handle_window_event_for_players(const window::Event& ev);
	void show_player_debug_window();

	bool kill_player(entt::entity entity);
}
