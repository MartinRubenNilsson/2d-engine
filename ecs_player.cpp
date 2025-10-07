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
#include "map_grid.h"
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

	enum INPUT_FLAGS : unsigned int {
		// Continuous actions

		INPUT_W = (1 << 0),
		INPUT_E = (1 << 1),
		INPUT_N = (1 << 2),
		INPUT_S = (1 << 3),
		INPUT_RUN = (1 << 4),
		INPUT_STEALTH = (1 << 5),

		// One-shot actions

		INPUT_INTERACT = (1 << 6),
		INPUT_SWING_SWORD = (1 << 7),
		INPUT_SHOOT_BOW = (1 << 8),
		INPUT_DROP_BOMB = (1 << 9),
	};

	enum class PlayerState {
		Normal,
		SwingingSword,
		ShootingBow,
		Dying,
		Dead
	};

	struct Player {
		Direction dir = Direction::S;
		unsigned int input_flags = 0;
		PlayerState state = PlayerState::Normal;
		Timer hurt_timer = { 1.f };
		int max_health = 3;
		int health = 3;
		int arrows = 10;
		int bombs = 5;
		int rupees = 10;
	};

	extern entt::registry _registry;
	unsigned int _input_flags_to_enable = 0;
	unsigned int _input_flags_to_disable = 0;

	void handle_window_event_for_players(const window::Event& ev) {
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
	}

	void _player_attack(entt::entity entity, const Vec2f& position) {
		Vec2f box_min = position - Vec2f(6.f, 6.f);
		Vec2f box_max = position + Vec2f(6.f, 6.f);
		deal_damage_in_box({ DamageType::Touch, 1, entity }, box_min, box_max, ~CC_Player);
	}

	void _on_player_begin_touch_pickup(entt::entity player_entity, entt::entity pickup_entity) {
		Player* player = _registry.try_get<Player>(player_entity);
		if (!player) return;

		switch (get_pickup_type(pickup_entity)) {
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
			player->state = PlayerState::Dying;
		}
		return true;
	}

	void _handle_physics_for_player(const PhysicsEvent& ev) {
		const Tag other_tag = get_tag(ev.other_entity);
		if (ev.type == PhysicsEventType::SensorBeginTouch) {
			if (other_tag == Tag::Pickup) {
				_on_player_begin_touch_pickup(ev.entity, ev.other_entity);
			}
		}
	}

	void _update_idle_player(entt::entity entity) {

	}

	void _emplace_player_state_machine(entt::entity entity) {
		StateMachine& sm = emplace_state_machine(entity);
		add_state(sm, {
			.id = "idle" });
		add_state(sm, {
			.id = "dying" });
		add_state(sm, {
			.id = "dead" });
	}

	void setup_players(MapId map) {
		const Vec2f map_size_in_pixels = get_size_in_pixels(map);

		for (auto [entity, object] : _registry.view<Type<Tag::Player>, ObjectId>().each()) {
			{
				player::Outfit outfit{};
				player::randomize_outfit(outfit);
				player::create_outfit_texture(outfit);
			}

			if (sprites::Sprite* sprite = get_sprite(entity)) {
				sprite->texture = graphics::get_framebuffer_texture(graphics::player_outfit_framebuffer);
			}

			emplace_tile_animation(entity);

			{
				Player& player = _registry.emplace<Player>(entity);
			}

			set_audio_listener(entity);
			set_physics_event_handler(entity, _handle_physics_for_player);
			set_damage_event_handler(entity, _handle_damage_for_player);

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

	void update_players(float dt) {
		const bool player_accepts_input = (dt > 0.f && window::has_focus() && !console::has_focus());

		for (auto [entity, player, body, tile, anim] :
			_registry.view<Player, b2BodyId, TileId, TileAnimation>().each()) {

			if (player_accepts_input) {
				player.input_flags |= _input_flags_to_enable;
				player.input_flags &= ~_input_flags_to_disable;
			} else {
				player.input_flags = 0;
			}

			// UPDATE TIMERS

			player.hurt_timer.update(dt);

			// GET PHYSICS STATE

			const Vec2f position = b2Body_GetWorldCenterOfMass(body);
			const Vec2f velocity = b2Body_GetLinearVelocity(body);
			Vec2f new_velocity; // will be modified differently depending on the state

			enum class HeldItemType {
				None,
				Sword,
				Bow,
			};

			HeldItemType held_item_type = HeldItemType::None;

			// UPDATE AUDIO

			audio::set_parameter_label("terrain", map::to_string(map::get_terrain_type_at(position)));
			//if (animation._frame_changed && animation._frame_id % 3 == 0) {
			//	// Take a step every 3 frames
			//	audio::create_event({ .path = "event:/snd_footstep" });
			//}

			// UPDATE POST-PROCESSING
			postprocessing::set_darkness_center(position);

			const Direction dir = player.dir;

			switch (dir) {
				case Direction::E: tile.flipped_horizontally = false; break;
				case Direction::W: tile.flipped_horizontally = true; break;
			}

			switch (player.state) {
				case PlayerState::Normal: {

					Vec2f new_move_dir;
					float new_move_speed = 0.f;

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

					new_velocity = new_move_dir * new_move_speed;

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
					} else if (new_move_speed >= _PLAYER_RUN_SPEED) {

						switch (dir) {
							case Direction::W: [[fallthrough]];
							case Direction::E: replace(tile, TILE_ID_PLAYER_RUN_E); break;
							case Direction::N: replace(tile, TILE_ID_PLAYER_RUN_N); break;
							case Direction::S: replace(tile, TILE_ID_PLAYER_RUN_S); break;
						}

						anim.set_loop(true);
						if (anim.looped() && (dir == Direction::N || dir == Direction::S)) {
							tile.flipped_horizontally = !tile.flipped_horizontally;
						}

					} else if (new_move_speed >= _PLAYER_WALK_SPEED) {

						switch (dir) {
							case Direction::W: [[fallthrough]];
							case Direction::E: replace(tile, TILE_ID_PLAYER_WALK_E); break;
							case Direction::N: replace(tile, TILE_ID_PLAYER_WALK_N); break;
							case Direction::S: replace(tile, TILE_ID_PLAYER_WALK_S); break;
						}

						anim.set_loop(true);
						if (anim.looped() && (dir == Direction::N || dir == Direction::S)) {
							tile.flipped_horizontally = !tile.flipped_horizontally;
						}

					} else {

						switch (dir) {
							case Direction::W: [[fallthrough]];
							case Direction::E: replace(tile, TILE_ID_PLAYER_IDLE_E); break;
							case Direction::N: replace(tile, TILE_ID_PLAYER_IDLE_N); break;
							case Direction::S: replace(tile, TILE_ID_PLAYER_IDLE_S); break;
						}

						anim.set_progress(0.f);
						anim.set_loop(false);
						//sprite.flags &= ~sprites::SPRITE_FLIP_VERTICALLY;
					}

					if (player.input_flags & INPUT_INTERACT) {
						const Vec2f box_center = position + to_unit(player.dir) * 16.f;
						const Vec2f box_min = box_center - Vec2f(6.f, 6.f);
						const Vec2f box_max = box_center + Vec2f(6.f, 6.f);
						interact_with_all_entities_in_box(box_min, box_max);
					}

				} break;
				case PlayerState::SwingingSword: {
					held_item_type = HeldItemType::Sword;
#if 0
					if (tile_dir != Direction::E) {
						sprite.flags &= ~sprites::SPRITE_FLIP_HORIZONTALLY;
					}
#endif
					// TODO
					//if (animation._frame_changed && animation._frame_id == 1) {
					//	_player_attack(player_entity, position + player.look_dir * 16.f);
					//}
					if (anim.done()) {
						player.state = PlayerState::Normal;
					}
				} break;
				case PlayerState::ShootingBow: {
					held_item_type = HeldItemType::Bow;
					if (dir != Direction::E) {
						tile.flipped_horizontally = false;
					}
#if 0
					if (player.arrows > 0 && anim.get_progress() > ???) {
						player.arrows--;
						create_arrow(position + to_unit(player.dir) * 16.f, to_unit(player.dir)* _PLAYER_ARROW_SPEED);
					}
#endif
					if (anim.done()) {
						player.state = PlayerState::Normal;
					}
				} break;
				case PlayerState::Dying: {

					const TileId original_tile = tile;

					switch (dir) {
						case Direction::W: [[fallthrough]];
						case Direction::E: replace(tile, TILE_ID_PLAYER_DYING_SE); break;
						case Direction::N: replace(tile, TILE_ID_PLAYER_DYING_NE); break;
						case Direction::S: replace(tile, TILE_ID_PLAYER_DYING_SE); break;
					}

					if (tile != original_tile) { // HACK
						anim.set_progress(0.f);
					}
					anim.set_loop(false);

					if (anim.done()) {

						switch (dir) {
							case Direction::W: [[fallthrough]];
							case Direction::E: replace(tile, TILE_ID_PLAYER_DEAD_SE); break;
							case Direction::N: replace(tile, TILE_ID_PLAYER_DEAD_NE); break;
							case Direction::S: replace(tile, TILE_ID_PLAYER_DEAD_SE); break;
						}

						kill_player(entity);
						player.state = PlayerState::Dead;
					}

				} break;
				case PlayerState::Dead: {
					// Do nothing, u r ded
				} break;
			}

			b2Body_SetLinearVelocity(body, new_velocity);

			// UPDATE HUD

			ui::bindings::hud_player_health = player.health;
			ui::bindings::hud_arrow_ammo = player.arrows;
			ui::bindings::hud_bomb_ammo = player.bombs;
			ui::bindings::hud_rupee_amount = player.rupees;

			// CLEAR ONE-SHOT INPUT FLAGS

			player.input_flags &= ~INPUT_INTERACT;
			player.input_flags &= ~INPUT_SWING_SWORD;
			player.input_flags &= ~INPUT_SHOOT_BOW;
			player.input_flags &= ~INPUT_DROP_BOMB;
		}

		// Blink while hurt.
		for (auto [entity, player, sprite] : _registry.view<Player, sprites::Sprite>().each()) {
			if (player.hurt_timer.running()) {
				constexpr float BLINK_PERIOD = 0.15f;
				float fraction = fmod(player.hurt_timer.get_time(), BLINK_PERIOD) / BLINK_PERIOD;
				sprite.color.a = (unsigned char)(255 * fraction);
			} else {
				sprite.color.a = 255;
			}
		}

		_input_flags_to_enable = 0;
		_input_flags_to_disable = 0;
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
			ImGui::Text("Terrain: %s", map::to_string(map::get_terrain_type_at(position)).c_str());
			ImGui::Text("State: %s", magic_enum::enum_name(player.state).data());
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

	bool kill_player(entt::entity entity) {
		if (!_registry.all_of<Player>(entity))
			return false;
		detach_camera(entity);
		audio::stop_all_in_bus();
		audio::create_event({ .path = "event:/snd_player_die" });
		audio::create_event({ .path = "event:/mus_coffin_dance" });
		ui::open_or_enqueue_textbox_presets("player/die");
		ui::bindings::hud_player_health = 0;
		return true;
	}
}
