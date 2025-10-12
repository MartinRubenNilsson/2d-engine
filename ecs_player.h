#pragma once

namespace ecs {
	void setup_players(MapId map);
	void patch_players(const struct Patch& patch);
	void update_players(float dt);
}
