#include "stdafx.h"
#include "ecs_player_types.h"
#include "ecs_states.h"
#include "ecs_tiled.h"
#include "ecs_animations.h"
#include "ecs_arrow.h"
#include "ecs_audio.h"
#include "input.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void _player_start_shooting(entt::entity entity) {
		b2Body_SetLinearVelocity(_registry.get<b2BodyId>(entity), Vec2f::ZERO); // stop moving
		const Direction dir = _registry.get<Direction>(entity);
		TileId& tile = _registry.get<TileId>(entity);
		replace(tile, dir, PLAYER_TILE_ID_BOW_DRAW_E, PLAYER_TILE_ID_BOW_DRAW_N, PLAYER_TILE_ID_BOW_DRAW_S);
		TileAnimation& anim = _registry.get<TileAnimation>(entity);
		anim.set_progress(0.f);
		anim.set_loop(false);
		const float anim_duration = get_animation_duration(tile);
		transition_to_state_later(entity, "normal", anim_duration);
		play_audio_at("event:/player/arrow/load", entity);
	}

	void _player_update_shooting(entt::entity entity, float dt) {
		Direction& dir = _registry.get<Direction>(entity);
		const Vec2f input_dir = input::get_dir();
		// Update direction.
		if (input_dir != Vec2f::ZERO) {
			dir = to_cardinal(input_dir);
		}
		TileId& tile = _registry.get<TileId>(entity);
		replace(tile, dir, PLAYER_TILE_ID_BOW_DRAW_E, PLAYER_TILE_ID_BOW_DRAW_N, PLAYER_TILE_ID_BOW_DRAW_S);
		const Vec2f unit_dir = to_unit(dir);
		const TileAnimation& anim = _registry.get<TileAnimation>(entity);
		// Check if the animation just arrived at the "release" frame.
		if (!anim.frame_changed()) return;
		const TileId frame = anim.get_tile();
		if (frame.id != PLAYER_TILE_ID_BOW_RELEASE_E &&
			frame.id != PLAYER_TILE_ID_BOW_RELEASE_N &&
			frame.id != PLAYER_TILE_ID_BOW_RELEASE_S) {
			return;
		}
		// Shoot arrow.
		Player& player = _registry.get<Player>(entity);
		const Vec2f player_pos = b2Body_GetWorldCenterOfMass(_registry.get<b2BodyId>(entity));
		const Vec2f arrow_pos = player_pos + unit_dir * 16.f;
		constexpr float ARROW_SPEED = 16.f * 20;
		const entt::entity arrow_entity = create_arrow(arrow_pos, unit_dir * ARROW_SPEED);
		if (arrow_entity == entt::null)
			return;
		player.arrows--;
		play_attached_audio("event:/player/arrow/shoot", arrow_entity);
	}

	StateId add_player_shooting_state(StateMachine& sm, StateId parent) {
		return add_state(sm, {
			.name = "shooting",
			.parent = parent,
			.start = _player_start_shooting,
			.update = _player_update_shooting });
	}
}