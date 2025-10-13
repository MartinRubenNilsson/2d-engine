#include "stdafx.h"
#include "ecs_player_types.h"
#include "ecs_states.h"
#include "sprites.h"

namespace ecs {
	extern entt::registry _registry;

	void _player_update_alive(entt::entity entity, float dt) {
		Player& player = _registry.get<Player>(entity);

		// Update invincibility time.
		if (player.invincibility_time > 0.f) {
			player.invincibility_time -= dt;
			if (player.invincibility_time <= 0.f) {
				player.invincibility_time = 0.f;
			}
		}

		// Blink sprite while invincible.
		sprites::Sprite& sprite = _registry.get<sprites::Sprite>(entity);
		if (player.invincibility_time > 0.f) {
			constexpr float BLINK_PERIOD = 0.15f;
			float fraction = fmod(player.invincibility_time, BLINK_PERIOD) / BLINK_PERIOD;
			sprite.color.a = (unsigned char)(255 * fraction);
		} else {
			sprite.color.a = 255;
		}
	}

	StateId add_player_alive_state(StateMachine& sm) {
		return add_state(sm, {
			.name = "alive",
			.update = _player_update_alive });
	}
}