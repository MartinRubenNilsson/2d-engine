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

	constexpr float _PLAYER_WALK_SPEED = 60.f;
	constexpr float _PLAYER_RUN_SPEED = 136.f;
	constexpr float _PLAYER_STEALTH_SPEED = 36.f;
	constexpr float _PLAYER_ARROW_SPEED = 160.f;

	struct Player {
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
				player->health = std::min(player->health + 1, player->max_health);
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
		player->health = std::max(0, player->health - ev.amount);
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
		TileId& tile = _registry.get<TileId>(entity);
		tile.flipped_horizontally = false;
		TileAnimation& anim = _registry.get<TileAnimation>(entity);
		anim.set_progress(0.f); // Reset animation
		anim.set_loop(true);
	}

	void _player_update_normal(entt::entity entity, float dt) {
		TileId& tile = _registry.get<TileId>(entity);
		const b2BodyId body = _registry.get<b2BodyId>(entity);
		const Direction dir = _registry.get<Direction>(entity);

		const Vec2f pos = b2Body_GetWorldCenterOfMass(body);
		const Vec2f vel = b2Body_GetLinearVelocity(body);
		const float speed = length(vel);

		// The tileset only has right-facing tiles, so we need to flip if we're facing left.
		switch (dir) {
			case Direction::E: tile.flipped_horizontally = false; break;
			case Direction::W: tile.flipped_horizontally = true; break;
		}

		if (speed >= _PLAYER_RUN_SPEED) {
			switch (dir) {
				case Direction::W: [[fallthrough]];
				case Direction::E: replace(tile, TILE_ID_PLAYER_RUN_E); break;
				case Direction::N: replace(tile, TILE_ID_PLAYER_RUN_N); break;
				case Direction::S: replace(tile, TILE_ID_PLAYER_RUN_S); break;
			}
		} else if (speed >= _PLAYER_WALK_SPEED) {
			switch (dir) {
				case Direction::W: [[fallthrough]];
				case Direction::E: replace(tile, TILE_ID_PLAYER_WALK_E); break;
				case Direction::N: replace(tile, TILE_ID_PLAYER_WALK_N); break;
				case Direction::S: replace(tile, TILE_ID_PLAYER_WALK_S); break;
			}
		} else {
			switch (dir) {
				case Direction::W: [[fallthrough]];
				case Direction::E: replace(tile, TILE_ID_PLAYER_IDLE_E); break;
				case Direction::N: replace(tile, TILE_ID_PLAYER_IDLE_N); break;
				case Direction::S: replace(tile, TILE_ID_PLAYER_IDLE_S); break;
			}
		}
		
		// Flip tile when facing up or down and animation loops to get a proper walk cycle.
		TileAnimation& anim = _registry.get<TileAnimation>(entity);
		if (anim.looped() && (dir == Direction::N || dir == Direction::S)) {
			tile.flipped_horizontally = !tile.flipped_horizontally;
		}
	}

	void _player_handle_normal(entt::entity entity, const StateEvent& ev) {
		if (ev.type != PLAYER_STATE_EVENT_KEY)
			return;
		const window::Event& window_ev = *(const window::Event*)ev.data;
		const b2BodyId body = _registry.get<b2BodyId>(entity);
		Direction& dir = _registry.get<Direction>(entity);


#if 0
		if (player.input_flags & INPUT_W)
			new_move_dir.x--;
		if (player.input_flags & INPUT_E)
			new_move_dir.x++;
		if (player.input_flags & INPUT_N)
			new_move_dir.y--;
		if (player.input_flags & INPUT_S)
			new_move_dir.y++;

		if (new_move_dir != Vec2f::ZERO) {
			player.dir = to_cardinal(new_move_dir);
			new_move_dir = normalize(new_move_dir);
			if (player.input_flags & INPUT_STEALTH) {
				new_move_speed = _PLAYER_STEALTH_SPEED;
				//} else if (player.touching_pushable_block) {
				//	new_move_speed = _PLAYER_WALK_SPEED;
			} else if (player.input_flags & INPUT_RUN) {
				new_move_speed = _PLAYER_RUN_SPEED;
			} else {
				new_move_speed = _PLAYER_WALK_SPEED;
			}
		}
#endif
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
				camera.confines_min = { 0.f, 0.f };
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

#if 0
		if (ev.type == window::EventType::KeyPress) {
			switch (ev.key.code) {
				case window::Key::Left:
					_input_flags_to_enable |= INPUT_W;
					break;
				case window::Key::Right:
					_input_flags_to_enable |= INPUT_E;
					break;
				case window::Key::Up:
					_input_flags_to_enable |= INPUT_N;
					break;
				case window::Key::Down:
					_input_flags_to_enable |= INPUT_S;
					break;
				case window::Key::LShift:
					_input_flags_to_enable |= INPUT_RUN;
					break;
				case window::Key::LControl:
					_input_flags_to_enable |= INPUT_STEALTH;
					break;
				case window::Key::C:
					_input_flags_to_enable |= INPUT_INTERACT;
					break;
				case window::Key::X:
					_input_flags_to_enable |= INPUT_SHOOT_BOW;
					break;
				case window::Key::Z:
					_input_flags_to_enable |= INPUT_DROP_BOMB;
					break;
				case window::Key::Space:
					_input_flags_to_enable |= INPUT_SWING_SWORD;
					break;
			}
		} else if (ev.type == window::EventType::KeyRelease) {
			switch (ev.key.code) {
				case window::Key::Left:
					_input_flags_to_disable |= INPUT_W;
					break;
				case window::Key::Right:
					_input_flags_to_disable |= INPUT_E;
					break;
				case window::Key::Up:
					_input_flags_to_disable |= INPUT_N;
					break;
				case window::Key::Down:
					_input_flags_to_disable |= INPUT_S;
					break;
				case window::Key::LShift:
					_input_flags_to_disable |= INPUT_RUN;
					break;
				case window::Key::LControl:
					_input_flags_to_disable |= INPUT_STEALTH;
					break;
			}
		}
#endif

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

					} else if (player.input_flags & INPUT_DROP_BOMB && player.bombs > 0) {

						const Vec2f bomb_position = position + to_unit(player.dir) * 16.f;
						if (create_bomb(bomb_position) != entt::null) {
							player.bombs--;
							audio::create_event({ .path = "event:/player/place_bomb" });
						} else {
							audio::create_event({ .path = "event:/player/error" });
						}

#if 0
					} else if (player.touching_pushable_block && new_velocity != Vec2f::ZERO) {

						switch (dir) {
							case Direction::W: [[fallthrough]];
							case Direction::E: replace(tile, TILE_ID_PLAYER_PUSH_E); break;
							case Direction::N: replace(tile, TILE_ID_PLAYER_PUSH_N); break;
							case Direction::S: replace(tile, TILE_ID_PLAYER_PUSH_S); break;
						}

						anim.set_loop(true);
						if (anim.looped() && (dir == Direction::N || dir == Direction::S)) {
							tile.flipped_horizontally = !tile.flipped_horizontally;
						}
#endif
					} else
					if (player.input_flags & INPUT_INTERACT) {
						const Vec2f box_center = position + to_unit(player.dir) * 16.f;
						const Vec2f box_min = box_center - Vec2f(6.f, 6.f);
						const Vec2f box_max = box_center + Vec2f(6.f, 6.f);
						interact_with_all_entities_in_box(box_min, box_max);
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
