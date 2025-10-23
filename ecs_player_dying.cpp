#include "stdafx.h"
#include "ecs_player_types.h"
#include "ecs_states.h"
#include "ecs_tiled.h"
#include "ecs_animations.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void _player_start_dying(entt::entity entity) {
		_registry.remove<b2BodyId>(entity); // Stop running physics.
		TileId& tile = _registry.get<TileId>(entity);
		const Direction dir = _registry.get<Direction>(entity);
		replace(tile, dir, PLAYER_TILE_ID_DYING_SE, PLAYER_TILE_ID_DYING_NE, PLAYER_TILE_ID_DYING_SE);
		audio::create_event("event:/player/die");
		TileAnimation& anim = _registry.get<TileAnimation>(entity);
		anim.set_progress(0.f);
		anim.set_loop(false);
		const float duration = get_animation_duration(tile);
		transition_to_state_later(entity, "dead", duration);
	}

	StateId add_player_dying_state(StateMachine& sm) {
		return add_state(sm, {
			.name = "dying",
			.start = _player_start_dying });
	}
}