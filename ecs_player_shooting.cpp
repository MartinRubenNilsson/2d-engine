#include "stdafx.h"
#include "ecs_player_types.h"
#include "ecs_states.h"
#include "ecs_tiled.h"
#include "ecs_animations.h"
#include "ecs_arrow.h"

namespace ecs {
	extern entt::registry _registry;

	void _player_start_shooting(entt::entity entity) {
		b2Body_SetLinearVelocity(_registry.get<b2BodyId>(entity), Vec2f::ZERO); // stop moving
		const Direction dir = _registry.get<Direction>(entity);
		TileId& tile = _registry.get<TileId>(entity);
		switch (dir) {
			case Direction::W: [[fallthrough]];
			case Direction::E: replace(tile, PLAYER_TILE_ID_BOW_DRAW_E); break;
			case Direction::N: replace(tile, PLAYER_TILE_ID_BOW_DRAW_N); break;
			case Direction::S: replace(tile, PLAYER_TILE_ID_BOW_DRAW_S); break;
		}
		TileAnimation& anim = _registry.get<TileAnimation>(entity);
		anim.set_progress(0.f);
		anim.set_loop(false);
		const float anim_duration = get_animation_duration(tile);
		transition_to_state_later(entity, "normal", anim_duration);
	}

	void _player_update_shooting(entt::entity entity, float dt) {
		const TileAnimation& anim = _registry.get<TileAnimation>(entity);
		// Check if the animation just arrived at the "release" frame.
		if (!anim.frame_changed()) return;
		const TileId frame = anim.get_frame();
		if (frame.id != PLAYER_TILE_ID_BOW_RELEASE_E &&
			frame.id != PLAYER_TILE_ID_BOW_RELEASE_N &&
			frame.id != PLAYER_TILE_ID_BOW_RELEASE_S) {
			return;
		}
		// Shoot arrow.
		Player& player = _registry.get<Player>(entity);
		const Vec2f pos = b2Body_GetWorldCenterOfMass(_registry.get<b2BodyId>(entity));
		const Vec2f dir = to_unit(_registry.get<Direction>(entity));
		constexpr float ARROW_SPEED = 16.f * 16;
		create_arrow(pos + dir * 16.f, dir * ARROW_SPEED);
		player.arrows--;
	}

	StateId add_player_shooting_state(StateMachine& sm, StateId parent) {
		return add_state(sm, {
			.name = "shooting",
			.parent = parent,
			.start = _player_start_shooting,
			.update = _player_update_shooting });
	}
}