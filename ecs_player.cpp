#include "stdafx.h"
#include "ecs_player.h"
#include "ecs_player_types.h"
#include "ecs_physics.h"
#include "ecs_physics_events.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"
#include "ecs_camera.h"
#include "ecs_bomb.h"
#include "ecs_damage.h"
#include "ecs_interactions.h"
#include "player_outfit.h"
#include "console.h"
#include "audio.h"
#include "ui_bindings.h"
#include "postprocessing.h"
#include "ecs_pickups.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_portal.h"
#include "ecs_lifetime.h"
#include "ecs_patch.h"
#include "graphics_globals.h"
#include "ecs_states.h"
#include "ecs_audio.h"
#include "ecs_terrain.h"
#include "ui.h"
#include "ecs_player_states.h"
#include "input.h"

namespace ecs {

	float get_desired_speed(PlayerMotion motion) {
		switch (motion) {
			default: return 0.f;
			case PlayerMotion::Walking: return 60.f;
			case PlayerMotion::Running: return 136.f;
			case PlayerMotion::Sneaking: return 36.f;
			case PlayerMotion::Pushing: return 16.f;
		}
	}

	extern entt::registry _registry;

	void _player_begin_touch_pickup(entt::entity player_entity, entt::entity pickup_entity) {
		Player* player = _registry.try_get<Player>(player_entity);
		if (!player) return;

		const PickupType type = get_pickup_type(pickup_entity);
		switch (type) {
			case PickupType::Arrow: {
				player->arrows++;
				audio::create_event({ .path = "event:/snd_pickup" });
			} break;
			case PickupType::Rupee: {
				player->rupees++;
				audio::create_event({ .path = "event:/snd_pickup_rupee" });
			} break;
			case PickupType::Bomb: {
				player->bombs++;
				audio::create_event({ .path = "event:/snd_pickup" });
			} break;
			case PickupType::Heart: {
				if (player->health < player->max_health) {
					player->health++;
				}
				//TODO
				//audio::play("event:/snd_pickup_heart");
			} break;
		}

		destroy_later(pickup_entity);
	}

	bool _player_handle_damage(entt::entity entity, const DamageEvent& ev) {
		if (ev.amount <= 0) return false;
		Player* player = _registry.try_get<Player>(entity);
		if (!player)
			return false;
		if (player->health <= 0)
			return false; // Player is already dead
		if (player->invincibility_time > 0.f)
			return false; // Player is invincible
		if (get_current_state(entity) == "hurt")
			return false; // Player is already in hurt state
		if (ev.amount <= player->health) {
			player->health -= ev.amount;
		} else {
			player->health = 0;
		}
		add_trauma_to_active_camera(0.8f); // TODO: broken?
		transition_to_state(entity, "hurt");
		return true;
	}

	void _player_handle_touch(const TouchEvent& ev) {
		if (ev.type == TouchEventType::SensorBegin) {
			const Tag other_tag = get_tag(ev.other_entity);
			if (other_tag == Tag::Pickup) {
				_player_begin_touch_pickup(ev.entity, ev.other_entity);
			}
		}
	}

	void _player_update_alive(entt::entity entity, float dt) {
		if (dt == 0.f)
			return; // Important, otherwise we can move during frozen time (e.g. when UI is open).

		Player& player = _registry.get<Player>(entity);

		// Update inputs.
		player.input_x = (float)input::held(input::Key::Right) - (float)input::held(input::Key::Left);
		player.input_y = (float)input::held(input::Key::Down) - (float)input::held(input::Key::Up);
		player.input_dir = normalize({ player.input_x, player.input_y });

		// Update invincibility time.
		if (player.invincibility_time > 0.f) {
			player.invincibility_time -= dt;
			if (player.invincibility_time <= 0.f) {
				player.invincibility_time = 0.f;
			}
		}

		// Update sprite.
		sprites::Sprite& sprite = _registry.get<sprites::Sprite>(entity);
		if (player.invincibility_time > 0.f) {
			constexpr float BLINK_PERIOD = 0.15f;
			float fraction = fmod(player.invincibility_time, BLINK_PERIOD) / BLINK_PERIOD;
			sprite.color.a = (unsigned char)(255 * fraction);
		} else {
			sprite.color.a = 255;
		}
	}

	void _player_start_normal(entt::entity entity) {
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

		// Update direction.
		if (player.input_dir != Vec2f::ZERO) {
			dir = to_cardinal(player.input_dir);
		}

		// Figure out if we're touching anything in the direction of motion.
		bool touching_something_in_dir = false;
		for (const b2ContactData& contact : get_contacts(body)) {
			const Direction contact_dir = to_cardinal(contact.manifold.normal);
			touching_something_in_dir = (contact_dir == dir);
		}

		// Update motion.
		player.motion = PlayerMotion::Motionless;
		if (player.input_dir != Vec2f::ZERO) {
			if (touching_something_in_dir) {
				player.motion = PlayerMotion::Pushing;
			} else if (input::held(input::Key::LControl)) {
				player.motion = PlayerMotion::Sneaking;
			} else if (input::held(input::Key::LShift)) {
				player.motion = PlayerMotion::Running;
			} else {
				player.motion = PlayerMotion::Walking;
			}
		}

		// Set desired velocity.
		const float desired_speed = get_desired_speed(player.motion);
		const Vec2f desired_vel = desired_speed * player.input_dir;
		b2Body_SetLinearVelocity(body, desired_vel);
		
		// Update tile depending on motion. Note that the tileset only has right-facing tiles
		// and that the north/south (up/down) animations typically only have half a walk cycle.
		// This means we sometimes have to flip the tile horizontally to cover all cases.
		switch (player.motion) {
			case PlayerMotion::Motionless: {
				switch (dir) {
					case Direction::W: [[fallthrough]];
					case Direction::E: replace(tile, PLAYER_TILE_ID_IDLE_E); break;
					case Direction::N: replace(tile, PLAYER_TILE_ID_IDLE_N); break;
					case Direction::S: replace(tile, PLAYER_TILE_ID_IDLE_S); break;
				}
			} break;
			case PlayerMotion::Walking: {
				switch (dir) {
					case Direction::W: [[fallthrough]];
					case Direction::E: replace(tile, PLAYER_TILE_ID_WALK_E); break;
					case Direction::N: replace(tile, PLAYER_TILE_ID_WALK_N); break;
					case Direction::S: replace(tile, PLAYER_TILE_ID_WALK_S); break;
				}
			} break;
			case PlayerMotion::Running: {
				switch (dir) {
					case Direction::W: [[fallthrough]];
					case Direction::E: replace(tile, PLAYER_TILE_ID_RUN_E); break;
					case Direction::N: replace(tile, PLAYER_TILE_ID_RUN_N); break;
					case Direction::S: replace(tile, PLAYER_TILE_ID_RUN_S); break;
				}
			} break;
			case PlayerMotion::Sneaking: {
				// TODO
			} break;
			case PlayerMotion::Pushing: {
				switch (dir) {
					case Direction::W: [[fallthrough]];
					case Direction::E: replace(tile, PLAYER_TILE_ID_PUSH_E); break;
					case Direction::N: replace(tile, PLAYER_TILE_ID_PUSH_N); break;
					case Direction::S: replace(tile, PLAYER_TILE_ID_PUSH_S); break;
				}
			} break;
		}

		// Update tile flip flags.
		if (dir == Direction::E) {
			tile.flipped_horizontally = false; // Never flip if we're facing east (right).
		} else if (dir == Direction::W) {
			tile.flipped_horizontally = true;  // Always flip if we're facing west (left).
		} else if (anim.looped()) {
			// Flip tile when facing north/south (up/down) and animation loops to get proper walk cycles.
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
		if (anim.looped()) { // TODO: need to also play sound halfway though left and right run anims
			Handle<audio::Event> ev = audio::create_event({ .path = "event:/snd_footstep" });
			audio::set_event_parameter_label(ev, "terrain", terrain);
		}

		// Update postprocessing. TODO: this should be in another place
		postprocessing::set_darkness_center(pos);

		// Should interact?
		if (input::pressed(input::Key::C)) {
			// Interact with everything one tile in front of the player.
			const Vec2f center = pos + player.input_dir * 16.f;
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
			const Vec2f bomb_pos = pos + player.input_dir * 16.f;
			if (create_bomb_at(bomb_pos) != entt::null) {
				player.bombs--;
				audio::create_event({ .path = "event:/player/place_bomb" });
			} else { // Failed to place bomb.
				audio::create_event({ .path = "event:/player/error" });
			}
		}
	}

	void _emplace_player_state_machine(entt::entity entity) {
		StateMachine& sm = emplace_state_machine(entity);
		StateId alive = add_state(sm, {
			.name = "alive",
			.update = _player_update_alive });
		StateId normal = add_state(sm, {
			.name = "normal",
			.parent = alive,
			.update = _player_update_normal });
		add_player_slashing_state(sm, alive);
		add_player_shooting_state(sm, alive);
		add_player_hurt_state(sm, alive);
		add_player_dying_state(sm);
		add_player_dead_state(sm);
		transition(sm, normal, entity);
	}

	void setup_players(MapId map) {
		const Vec2f map_size_in_pixels = get_size_in_pixels(map);

		for (auto [entity, object, sprite] : _registry.view<Type<Tag::Player>, ObjectId, sprites::Sprite>().each()) {
			_registry.emplace<Player>(entity);
			_registry.emplace<Direction>(entity, Direction::S);
			
			{
				player::Outfit outfit{};
				player::randomize_outfit(outfit);
				player::create_outfit_texture(outfit);
			}

			sprite.texture = graphics::get_framebuffer_texture(graphics::player_outfit_framebuffer);

			emplace_tile_animation(entity);

			set_audio_listener(entity);
			set_touch_event_handler(entity, _player_handle_touch);
			set_damage_event_handler(entity, _player_handle_damage);

			_emplace_player_state_machine(entity);

			{
				Camera camera{};
				camera.confines_min = Vec2f::ZERO;
				camera.confines_max = map_size_in_pixels;
				camera.entity_to_follow = entity;
				emplace_camera(entity, camera);
				activate_camera(entity, true);
			}
		}
	}

	void _teleport_players_to_portal(std::string_view portal_name) {
		entt::entity portal_entity = get_portal_with_name(portal_name);
		if (portal_entity == entt::null) return;
		for (auto [player_entity] : _registry.view<Type<Tag::Player>>().each()) {
			teleport_entity_to_portal(player_entity, portal_entity);
		}
	}

	void patch_players(const Patch& patch) {
		if (!patch.portal_to_exit.empty()) {
			_teleport_players_to_portal(patch.portal_to_exit);
		}
	}

	void _dispatch_window_events_to_players() {
		if (ui::is_menu_or_textbox_visible())
			return;
	}

	void update_players(float dt) {

		// Update hud. TODO: put in ecs_ui_hud.h or something
		for (auto [entity, player] : _registry.view<Player>().each()) {
			ui::bindings::hud_player_health = player.health;
			ui::bindings::hud_arrow_ammo = player.arrows;
			ui::bindings::hud_bomb_ammo = player.bombs;
			ui::bindings::hud_rupee_amount = player.rupees;
		}
	}
}
