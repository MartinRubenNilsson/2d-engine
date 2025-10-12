#include "stdafx.h"
#include "ecs_player.h"
#include "ecs_states.h"
#include "ecs_tiled.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void _player_start_hurt(entt::entity entity) {
		b2Body_SetLinearVelocity(_registry.get<b2BodyId>(entity), Vec2f::ZERO); // Stop moving
		replace(_registry.get<TileId>(entity), PLAYER_TILE_ID_HURT_S);
		audio::create_event({ .path = "event:/player/hurt" });
		Player& player = _registry.get<Player>(entity);
		if (player.health > 0) {
			transition_to_state_later(entity, "normal", 0.2f);
		} else {
			transition_to_state_later(entity, "dying", 0.4f);
		}
	}

	void _player_stop_hurt(entt::entity entity) {
		Player& player = _registry.get<Player>(entity);
		player.invincibility_time = 1.f; // Become invincible for a time.
	}

	StateId add_player_hurt_state(StateMachine& sm, StateId parent) {
		return add_state(sm, {
			.name = "hurt",
			.parent = parent,
			.start = _player_start_hurt,
			.stop = _player_stop_hurt });
	}
}