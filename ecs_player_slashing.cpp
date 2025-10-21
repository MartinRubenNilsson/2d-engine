#include "stdafx.h"
#include "ecs_player_types.h"
#include "ecs_states.h"
#include "ecs_tiled.h"
#include "ecs_animations.h"
#include "ecs_damage.h"
#include "ecs_physics_filters.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void _player_start_slashing(entt::entity entity) {
		b2Body_SetLinearVelocity(_registry.get<b2BodyId>(entity), Vec2f::ZERO); // stop moving
		const Direction dir = _registry.get<Direction>(entity);
		TileId& tile = _registry.get<TileId>(entity);
		replace(tile, dir, PLAYER_TILE_ID_SLASH_E, PLAYER_TILE_ID_SLASH_N, PLAYER_TILE_ID_SLASH_S);
		TileAnimation& anim = _registry.get<TileAnimation>(entity);
		anim.set_progress(0.f);
		anim.set_loop(false);
		audio::create_event({ .path = "event:/snd_sword_attack" });
		const float anim_duration = get_animation_duration(tile);
		transition_to_state_later(entity, "normal", anim_duration);
	}

	void _player_deal_slash_damage(entt::entity entity, const Vec2f& position) {
		Vec2f box_min = position - Vec2f(6.f, 6.f);
		Vec2f box_max = position + Vec2f(6.f, 6.f);
		deal_damage_in_box({ DamageType::Melee, 1, entity }, box_min, box_max, ~CC_Player);
	}

	void _player_update_slashing(entt::entity entity, float dt) {
		const TileAnimation& anim = _registry.get<TileAnimation>(entity);
		// Check if the animation just arrived at the "done" frame.
		if (!anim.tile_changed()) return;
		const TileId frame = anim.get_tile();
		if (frame.id != PLAYER_TILE_ID_SLASH_DONE_E &&
			frame.id != PLAYER_TILE_ID_SLASH_DONE_N &&
			frame.id != PLAYER_TILE_ID_SLASH_DONE_S) {
			return;
		}
		Player& player = _registry.get<Player>(entity);
		const Vec2f pos = b2Body_GetWorldCenterOfMass(_registry.get<b2BodyId>(entity));
		const Vec2f dir = to_unit(_registry.get<Direction>(entity));
		_player_deal_slash_damage(entity, pos + dir * 16.f);
	}

	StateId add_player_slashing_state(StateMachine& sm, StateId parent) {
		return add_state(sm, {
			.name = "slashing",
			.parent = parent,
			.start = _player_start_slashing,
			.update = _player_update_slashing });
	}
}