#include "stdafx.h"
#include "ecs_player_types.h"
#include "ecs_states.h"
#include "ecs_tags.h"
#include "ecs_animations.h"
#include "ecs_physics.h"
#include "ecs_bomb.h"
#include "ecs_interactions.h"
#include "postprocessing.h"
#include "ecs_terrain.h"
#include "input.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	float _get_desired_speed(PlayerMotion motion) {
		switch (motion) {
			default: return 0.f;
			case PlayerMotion::Walking: return 80.f;
			case PlayerMotion::Running: return 160.f;
			case PlayerMotion::Sneaking: return 36.f;
			case PlayerMotion::Pushing: return 16.f;
		}
	}

	void _player_update_normal(entt::entity entity, float dt) {

		Player& player = _registry.get<Player>(entity);
		b2BodyId& body = _registry.get<b2BodyId>(entity);
		Direction& dir = _registry.get<Direction>(entity);
		TileId& tile = _registry.get<TileId>(entity);
		TileAnimation& anim = _registry.get<TileAnimation>(entity);

		const Vec2f pos = b2Body_GetWorldCenterOfMass(body);
		const Vec2f vel = b2Body_GetLinearVelocity(body);
		const float speed = length(vel);

		const Vec2f input_dir = input::get_dir();

		// Update direction.
		if (input_dir != Vec2f::ZERO) {
			dir = to_cardinal(input_dir);
		}

		// Figure out if we're touching anything in the direction of motion.
		bool touching_something_in_dir = false;
		for (const b2ContactData& contact : get_contacts(body)) {
			const Tag other_tag = get_tag(get_entity(b2Shape_GetBody(contact.shapeIdB)));
			if (other_tag == Tag::Bounds) continue;
			const Direction contact_dir = to_cardinal(contact.manifold.normal);
			touching_something_in_dir = (contact_dir == dir);
		}

		// Update motion.
		player.motion = PlayerMotion::Motionless;
		if (input_dir != Vec2f::ZERO) {
			if (touching_something_in_dir) {
				player.motion = PlayerMotion::Pushing;
			} else if (input::down(input::Key::LControl)) {
				player.motion = PlayerMotion::Sneaking;
			} else if (input::down(input::Key::LShift)) {
				player.motion = PlayerMotion::Running;
			} else {
				player.motion = PlayerMotion::Walking;
			}
		}

		// Set desired velocity.
		const float desired_speed = _get_desired_speed(player.motion);
		const Vec2f desired_vel = desired_speed * input_dir;
		b2Body_SetLinearVelocity(body, desired_vel);

		// Update tile depending on motion. Note that the tileset only has right-facing tiles
		// and that the north/south (up/down) animations typically only have half a walk cycle.
		// This means we sometimes have to flip the tile horizontally to cover all cases.
		switch (player.motion) {
			case PlayerMotion::Motionless: {
				replace(tile, dir, PLAYER_TILE_ID_IDLE_E, PLAYER_TILE_ID_IDLE_N, PLAYER_TILE_ID_IDLE_S);
			} break;
			case PlayerMotion::Walking: {
				replace(tile, dir, PLAYER_TILE_ID_WALK_E, PLAYER_TILE_ID_WALK_N, PLAYER_TILE_ID_WALK_S);
			} break;
			case PlayerMotion::Running: {
				replace(tile, dir, PLAYER_TILE_ID_RUN_E, PLAYER_TILE_ID_RUN_N, PLAYER_TILE_ID_RUN_S);
			} break;
			case PlayerMotion::Sneaking: {
				// TODO
			} break;
			case PlayerMotion::Pushing: {
				replace(tile, dir, PLAYER_TILE_ID_PUSH_E, PLAYER_TILE_ID_PUSH_N, PLAYER_TILE_ID_PUSH_S);
			} break;
		}

		// Flip tile when facing north/south (up/down) and animation loops to get proper walk cycles.
		if (anim.looped()) {
			tile.flipped_horizontally = !tile.flipped_horizontally;
		}

		// Update animation.
		if (player.motion == PlayerMotion::Motionless) {
			anim.set_progress(0.f);
			anim.set_loop(false);
		} else {
			// PITFALL: Make sure to not reset the progress while moving so the animation
			// "remembers" where it was in the walk cycle even if we change motion halfway.
			anim.set_loop(true);
		}

		// Play footstep sounds.
		const std::string terrain(to_string(get_terrain_at(pos)));
		if (anim.frame_changed() && anim.get_frame() < UINT_MAX && anim.get_frame() % 3 == 0) {
			Handle<audio::Event> ev = audio::create_event("event:/snd_footstep");
			audio::set_parameter_label(ev, "terrain", terrain);
		}

		// Update postprocessing. TODO: this should be in another place
		postprocessing::set_darkness_center(pos);

		// Should interact?
		if (input::pressed(input::Key::C)) {
			// Interact with everything one tile in front of the player.
			const Vec2f center = pos + input_dir * 16.f;
			const Vec2f min = center - Vec2f(6.f, 6.f);
			const Vec2f max = center + Vec2f(6.f, 6.f);
			interact_with_all_in_box(min, max);
		}

		// Should attack?
		if (input::pressed(input::Key::Space)) {
			transition_to_state(entity, "slashing");
		}

		// Should shoot bow?
		if (input::pressed(input::Key::X) && player.arrows > 0) {
			transition_to_state(entity, "shooting");
		}

		// Should place bomb?
		if (input::pressed(input::Key::Z) && player.bombs > 0) {
			// Place a bomb one tile in front of the player.
			const Vec2f bomb_pos = pos + to_unit(dir) * 16.f;
			if (create_bomb_at(bomb_pos) != entt::null) {
				player.bombs--;
				audio::create_event("event:/player/place_bomb");
			} else { // Failed to place bomb.
				audio::create_event("event:/player/error");
			}
		}
	}

	StateId add_player_normal_state(StateMachine& sm, StateId parent) {
		return add_state(sm, {
			.name = "normal",
			.parent = parent,
			.update = _player_update_normal });
	}
}