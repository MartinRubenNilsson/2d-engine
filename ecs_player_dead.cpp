#include "stdafx.h"
#include "ecs_player_types.h"
#include "ecs_states.h"
#include "ecs_tiled.h"
#include "ecs_camera.h"
#include "ui_textbox.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void _player_start_dead(entt::entity entity) {
		TileId& tile = _registry.get<TileId>(entity);
		const Direction dir = _registry.get<Direction>(entity);
		replace(tile, dir, PLAYER_TILE_ID_DEAD_SE, PLAYER_TILE_ID_DEAD_NE, PLAYER_TILE_ID_DEAD_SE);
		detach_camera(entity);
		audio::stop_all_in_bus();
		audio::create_event({ .path = "event:/music/death_jingle" });
		ui::textbox::open_next("player/die");
	}

	StateId add_player_dead_state(StateMachine& sm) {
		return add_state(sm, {
			.name = "dead",
			.start = _player_start_dead });
	}
}