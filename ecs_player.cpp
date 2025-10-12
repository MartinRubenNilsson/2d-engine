#include "stdafx.h"
#include "ecs_player.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_physics_events.h"
#include "ecs_arrow.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"
#include "ecs_camera.h"
#include "ecs_bomb.h"
#include "ecs_damage.h"
#include "ecs_interactions.h"
#include "player_outfit.h"
#include "console.h"
#include "audio.h"
#include "window.h"
#include "window_events.h"
#include "ui_textbox.h"
#include "ui_bindings.h"
#include "postprocessing.h"
#include "ecs_pickups.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_portal.h"
#include "ecs_lifetime.h"
#include "ecs_patch.h"
#include "graphics_globals.h"
#include "timer.h"
#include "ecs_states.h"
#include "ecs_audio.h"
#include "ecs_terrain.h"
#include "ui.h"

namespace ecs {

	// player.tsx
	enum TILE_ID_PLAYER {
		TILE_ID_PLAYER_IDLE_S = 0,
		TILE_ID_PLAYER_IDLE_N = 16,
		TILE_ID_PLAYER_IDLE_E = 32,
		TILE_ID_PLAYER_PUSH_S = 8,
		TILE_ID_PLAYER_PUSH_N = 24,
		TILE_ID_PLAYER_PUSH_E = 40,
		TILE_ID_PLAYER_WALK_S = 48,
		TILE_ID_PLAYER_WALK_N = 52,
		TILE_ID_PLAYER_WALK_E = 64,
		TILE_ID_PLAYER_RUN_S = 51,
		TILE_ID_PLAYER_RUN_N = 55,
		TILE_ID_PLAYER_RUN_E = 70,
		TILE_ID_PLAYER_FOREHAND_STRIKE_S = 132,
		TILE_ID_PLAYER_FOREHAND_STRIKE_N = 148,
		TILE_ID_PLAYER_FOREHAND_STRIKE_E = 164,
		TILE_ID_PLAYER_BOW_SHOT_S = 133,
		TILE_ID_PLAYER_BOW_SHOT_N = 149,
		TILE_ID_PLAYER_BOW_SHOT_E = 165,
		TILE_ID_PLAYER_DYING_SE = 178,
		TILE_ID_PLAYER_DYING_NE = 181,
		TILE_ID_PLAYER_DEAD_SE = 180,
		TILE_ID_PLAYER_DEAD_NE = 183,
	};

	constexpr float _PLAYER_ARROW_SPEED = 160.f;

	enum class PlayerMotion {
		Motionless,
		Walking,
		Running,
		Sneaking,
		Pushing
	};

	float get_desired_speed(PlayerMotion motion) {
		switch (motion) {
			default: return 0.f;
			case PlayerMotion::Walking: return 60.f;
			case PlayerMotion::Running: return 136.f;
			case PlayerMotion::Sneaking: return 36.f;
			case PlayerMotion::Pushing: return 16.f;
		}
	}

	struct Player {
		bool holding_up_key = false;
		bool holding_down_key = false;
		bool holding_left_key = false;
		bool holding_right_key = false;
		bool holding_shift_key = false;
		bool holding_control_key = false;

		float input_x = 0.f; // one of -1, 0, 1
		float input_y = 0.f; // one of -1, 0, 1
		Vec2f input_dir = Vec2f::ZERO; // either zero or an unit vector

		PlayerMotion motion = PlayerMotion::Motionless;

		Timer hurt_timer = { 1.f };
		int max_health = 3;
		int health = 3;
		int arrows = 10;
		int bombs = 5;
		int rupees = 10;
	};

	extern entt::registry _registry;

	void _player_attack(entt::entity entity, const Vec2f& position) {
		Vec2f box_min = position - Vec2f(6.f, 6.f);
		Vec2f box_max = position + Vec2f(6.f, 6.f);
		deal_damage_in_box({ DamageType::Touch, 1, entity }, box_min, box_max, ~CC_Player);
	}

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

	bool _handle_damage_for_player(entt::entity entity, const DamageEvent& ev) {
		if (ev.amount <= 0) return false;
		Player* player = _registry.try_get<Player>(entity);
		if (!player) return false;
		if (player->health <= 0) return false; // Player is already dead
		if (player->hurt_timer.running()) return false; // Player is invulnerable
		if (ev.amount <= player->health) {
			player->health -= ev.amount;
		} else {
			player->health = 0;
		}
		add_trauma_to_active_camera(0.8f);
		if (player->health > 0) { // Player survived
			audio::create_event({ .path = "event:/snd_player_hurt" });
			player->hurt_timer.start();
		} else { // Player died
			transition_to_state(entity, "dying");
		}
		return true;
	}

	void _handle_physics_for_player(const PhysicsEvent& ev) {
		const Tag other_tag = get_tag(ev.other_entity);
		if (ev.type == PhysicsEventType::SensorBeginTouch) {
			if (other_tag == Tag::Pickup) {
				_player_begin_touch_pickup(ev.entity, ev.other_entity);
			}
		}
	}

	enum PLAYER_STATE_EVENT {
		PLAYER_STATE_EVENT_KEY // data = window::Event
	};

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

		// Update motion.
		player.motion = PlayerMotion::Motionless;
		if (player.input_dir != Vec2f::ZERO) {
			if (player.holding_control_key) {
				player.motion = PlayerMotion::Sneaking;
			} else if (player.holding_shift_key) {
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
					case Direction::E: replace(tile, TILE_ID_PLAYER_IDLE_E); break;
					case Direction::N: replace(tile, TILE_ID_PLAYER_IDLE_N); break;
					case Direction::S: replace(tile, TILE_ID_PLAYER_IDLE_S); break;
				}
			} break;
			case PlayerMotion::Walking: {
				switch (dir) {
					case Direction::W: [[fallthrough]];
					case Direction::E: replace(tile, TILE_ID_PLAYER_WALK_E); break;
					case Direction::N: replace(tile, TILE_ID_PLAYER_WALK_N); break;
					case Direction::S: replace(tile, TILE_ID_PLAYER_WALK_S); break;
				}
			} break;
			case PlayerMotion::Running: {
				switch (dir) {
					case Direction::W: [[fallthrough]];
					case Direction::E: replace(tile, TILE_ID_PLAYER_RUN_E); break;
					case Direction::N: replace(tile, TILE_ID_PLAYER_RUN_N); break;
					case Direction::S: replace(tile, TILE_ID_PLAYER_RUN_S); break;
				}
			} break;
			case PlayerMotion::Sneaking: {
				// TODO
			} break;
			case PlayerMotion::Pushing: {
				switch (dir) {
					case Direction::W: [[fallthrough]];
					case Direction::E: replace(tile, TILE_ID_PLAYER_PUSH_E); break;
					case Direction::N: replace(tile, TILE_ID_PLAYER_PUSH_N); break;
					case Direction::S: replace(tile, TILE_ID_PLAYER_PUSH_S); break;
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
	}

	void _player_handle_normal_window_event(entt::entity entity, const window::Event& ev) {

		Player& player = _registry.get<Player>(entity);
		b2BodyId& body = _registry.get<b2BodyId>(entity);

		// Update held keys.
		const bool key_press = (ev.type == window::EventType::KeyPress); // Otherwise release
		switch (ev.key.code) {
			case window::Key::Up:
				player.holding_up_key = key_press;
				break;
			case window::Key::Down:
				player.holding_down_key = key_press;
				break;
			case window::Key::Left:
				player.holding_left_key = key_press;
				break;
			case window::Key::Right:
				player.holding_right_key = key_press;
				break;
			case window::Key::LShift:
				player.holding_shift_key = key_press;
				break;
			case window::Key::LControl:
				player.holding_control_key = key_press;
				break;
		}

		// Update inputs.
		player.input_x = (float)player.holding_right_key - (float)player.holding_left_key;
		player.input_y = (float)player.holding_down_key - (float)player.holding_up_key;
		player.input_dir = normalize({ player.input_x, player.input_y });
		
		const Vec2f pos = b2Body_GetWorldCenterOfMass(body);

		// Should interact?
		if (ev.type == window::EventType::KeyPress && ev.key.code == window::Key::C) {
			// Interact with everything one tile in front of the player.
			const Vec2f center = pos + player.input_dir * 16.f;
			const Vec2f min = center - Vec2f(6.f, 6.f);
			const Vec2f max = center + Vec2f(6.f, 6.f);
			interact_with_all_in_box(min, max);
		}

		// Should attack?
		if (ev.type == window::EventType::KeyPress && ev.key.code == window::Key::Space) {
			// TODO: swing sword
		}

		// Should shoot bow?
		if (ev.type == window::EventType::KeyPress && ev.key.code == window::Key::X) {
			// TODO: shoot bow
		}

		// Should place bomb?
		if (ev.type == window::EventType::KeyPress && ev.key.code == window::Key::Z && player.bombs > 0) {
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

	void _player_handle_normal(entt::entity entity, const StateEvent& ev) {
		if (ev.type == PLAYER_STATE_EVENT_KEY) {
			_player_handle_normal_window_event(entity, *(const window::Event*)ev.data);
		}
	}

	void _player_start_dying(entt::entity entity) {
		remove_body(entity);
		TileId& tile = _registry.get<TileId>(entity);
		const Direction dir = _registry.get<Direction>(entity);
		switch (dir) {
			case Direction::W: [[fallthrough]];
			case Direction::E: replace(tile, TILE_ID_PLAYER_DYING_SE); break;
			case Direction::N: replace(tile, TILE_ID_PLAYER_DYING_NE); break;
			case Direction::S: replace(tile, TILE_ID_PLAYER_DYING_SE); break;
		}
		TileAnimation& anim = _registry.get<TileAnimation>(entity);
		anim.set_progress(0.f);
		anim.set_loop(false);
		const float anim_duration = get_animation_duration(tile);
		transition_to_state_later(entity, "dead", anim_duration);
	}

	void _player_start_dead(entt::entity entity) {
		TileId& tile = _registry.get<TileId>(entity);
		const Direction dir = _registry.get<Direction>(entity);
		switch (dir) {
			case Direction::W: [[fallthrough]];
			case Direction::E: replace(tile, TILE_ID_PLAYER_DEAD_SE); break;
			case Direction::N: replace(tile, TILE_ID_PLAYER_DEAD_NE); break;
			case Direction::S: replace(tile, TILE_ID_PLAYER_DEAD_SE); break;
		}
		detach_camera(entity);
		audio::stop_all_in_bus();
		audio::create_event({ .path = "event:/snd_player_die" });
		audio::create_event({ .path = "event:/mus_coffin_dance" });
		ui::open_or_enqueue_textbox_presets("player/die");
	}

	void _emplace_player_state_machine(entt::entity entity) {
		StateMachine& sm = emplace_state_machine(entity);
		StateId normal = add_state(sm, {
			.name = "normal",
			.update = _player_update_normal,
			.handle = _player_handle_normal });
		add_state(sm, {
			.name = "dying",
			.start = _player_start_dying });
		add_state(sm, {
			.name = "dead",
			.start = _player_start_dead });
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
			set_physics_event_handler(entity, _handle_physics_for_player);
			set_damage_event_handler(entity, _handle_damage_for_player);

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
		if (!window::has_focus())
			return;
		if (console::has_focus())
			return;
		if (ui::is_menu_or_textbox_visible())
			return;

		const std::span<const window::Event> events = window::get_events();

		for (auto [entity] : _registry.view<Type<Tag::Player>>().each()) {
			for (const window::Event& ev : events) {
				if (ev.type != window::EventType::KeyPress &&
					ev.type != window::EventType::KeyRelease) {
					continue;
				}
				// Dispatch the window event to the player state machine.
				StateEvent state_ev{};
				state_ev.type = PLAYER_STATE_EVENT_KEY;
				state_ev.data = &ev;
				handle(entity, state_ev);
			}
		}
	}

	void update_players(float dt) {
		if (dt > 0.f) {
			_dispatch_window_events_to_players();
		}

		for (auto [entity, player] : _registry.view<Player>().each()) {
			player.hurt_timer.update(dt);

#if 0
			// UPDATE AUDIO
			{
				std::string terrain(to_string(get_terrain_at(position)));
				//audio::set_parameter_label("terrain", terrain);
				if (anim.looped()) {
					// Take a step every 3 loop
					Handle<audio::Event> ev = audio::create_event({ .path = "event:/snd_footstep" });
					audio::set_event_parameter_label(ev, "terrain", terrain);
				}
			}

			// UPDATE POST-PROCESSING
			postprocessing::set_darkness_center(position);

			const Direction dir = player.dir;

			switch (player.state) {
				case PlayerState::Normal: {

					if (player.input_flags & INPUT_SWING_SWORD) {

						switch (dir) {
							case Direction::W: [[fallthrough]];
							case Direction::E: replace(tile, TILE_ID_PLAYER_FOREHAND_STRIKE_E); break;
							case Direction::N: replace(tile, TILE_ID_PLAYER_FOREHAND_STRIKE_N); break;
							case Direction::S: replace(tile, TILE_ID_PLAYER_FOREHAND_STRIKE_S); break;
						}

						audio::create_event({ .path = "event:/snd_sword_attack" });
						anim.set_progress(0.f);
						anim.set_loop(false);

						player.state = PlayerState::SwingingSword;

					} else if (player.input_flags & INPUT_SHOOT_BOW && player.arrows > 0) {

						switch (dir) {
							case Direction::W: [[fallthrough]];
							case Direction::E: replace(tile, TILE_ID_PLAYER_BOW_SHOT_E); break;
							case Direction::N: replace(tile, TILE_ID_PLAYER_BOW_SHOT_N); break;
							case Direction::S: replace(tile, TILE_ID_PLAYER_BOW_SHOT_S); break;
						}

						anim.set_progress(0.f);
						anim.set_loop(false);

						player.state = PlayerState::ShootingBow;

					}

				} break;
				case PlayerState::SwingingSword: {
					if (tile_dir != Direction::E) {
						sprite.flags &= ~sprites::SPRITE_FLIP_HORIZONTALLY;
					}
					// TODO
					//if (animation._frame_changed && animation._frame_id == 1) {
					//	_player_attack(player_entity, position + player.look_dir * 16.f);
					//}
					if (anim.done()) {
						player.state = PlayerState::Normal;
					}
				} break;
				case PlayerState::ShootingBow: {
					if (dir != Direction::E) {
						tile.flipped_horizontally = false;
					}
					if (player.arrows > 0 && anim.get_progress() > ???) {
						player.arrows--;
						create_arrow(position + to_unit(player.dir) * 16.f, to_unit(player.dir)* _PLAYER_ARROW_SPEED);
					}
					if (anim.done()) {
						player.state = PlayerState::Normal;
					}
				} break;
			}
#endif
		}

		// Update hud. TODO: put in ecs_ui_hud.h or something
		for (auto [entity, player] : _registry.view<Player>().each()) {
			ui::bindings::hud_player_health = player.health;
			ui::bindings::hud_arrow_ammo = player.arrows;
			ui::bindings::hud_bomb_ammo = player.bombs;
			ui::bindings::hud_rupee_amount = player.rupees;
		}

		// Update graphics.
		for (auto [entity, player, tile, sprite] : _registry.view<Player, TileId, sprites::Sprite>().each()) {

			const std::string_view state = get_current_state(entity);

			if (player.hurt_timer.running()) {
				constexpr float BLINK_PERIOD = 0.15f;
				float fraction = fmod(player.hurt_timer.get_time(), BLINK_PERIOD) / BLINK_PERIOD;
				sprite.color.a = (unsigned char)(255 * fraction);
			} else {
				sprite.color.a = 255;
			}
		}
	}

	void show_player_debug_window() {
#ifdef _DEBUG_IMGUI
		const size_t player_count = _registry.view<Player>().size();

		ImGui::Begin("Players");
		for (auto [entity, player, body, animation] : _registry.view<Player, b2BodyId, TileAnimation>().each()) {
			const std::string tree_node_label = "Player " + std::to_string((uint32_t)entity);

			// For convenience, if there's just one player, open debug menu immediately. 
			ImGui::SetNextItemOpen(player_count == 1, ImGuiCond_Appearing);
			if (!ImGui::TreeNode(tree_node_label.c_str())) continue;

			Vec2f position = b2Body_GetWorldCenterOfMass(body);
			ImGui::Text("Position: %.1f, %.1f", position.x, position.y);
			ImGui::Text("Terrain: %s", to_string(get_terrain_at(position)).data());
			ImGui::Text("Health: %d", player.health);
			ImGui::Text("Arrows: %d", player.arrows);
			ImGui::Text("Rupees: %d", player.rupees);

			if (ImGui::Button("Apply 1 Damage")) {
				_handle_damage_for_player(entity, { DamageType::Default, 1 });
			}
			if (ImGui::Button("Kill")) {
				_handle_damage_for_player(entity, { DamageType::Default, 999 });
			}

			if (ImGui::Button("Give 5 Arrows")) {
				player.arrows += 5;
			}
			if (ImGui::Button("Give 5 Bombs")) {
				player.bombs += 5;
			}
			if (ImGui::Button("Give 5 Rupees")) {
				player.rupees += 5;
			}

#if 0
			if (ImGui::Button("Randomize Outfit")) {
				randomize_outfit(character);
				create_outfit_texture(character);
				animation.texture = character.texture;
			}
#endif

			ImGui::Spacing(); //did this even do anything??
			ImGui::TreePop();
		}

		ImGui::End();
#endif // _DEBUG_IMGUI
	}
}
