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
		switch (dir) {
			case Direction::W: [[fallthrough]];
			case Direction::E: replace(tile, PLAYER_TILE_ID_DEAD_SE); break;
			case Direction::N: replace(tile, PLAYER_TILE_ID_DEAD_NE); break;
			case Direction::S: replace(tile, PLAYER_TILE_ID_DEAD_SE); break;
		}
		detach_camera(entity);
		audio::stop_all_in_bus();
		audio::create_event({ .path = "event:/music/death_jingle" });
		ui::open_or_enqueue_textbox_presets("player/die");
	}

	StateId add_player_dead_state(StateMachine& sm) {
		return add_state(sm, {
			.name = "dead",
			.start = _player_start_dead });
	}
}