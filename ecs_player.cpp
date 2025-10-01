#include "stdafx.h"
#include "ecs_player.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_arrow.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"
#include "ecs_camera.h"
#include "ecs_bomb.h"
#include "ecs_damage.h"
#include "ecs_interactions.h"
#include "ecs_portal.h"
#include "player_outfit.h"
#include "console.h"
#include "audio.h"
#include "window.h"
#include "window_events.h"
#include "ui_hud.h"
#include "ui_textbox.h"
#include "ui_bindings.h"
#include "random.h"
#include "map_grid.h"
#include "postprocessing.h"
#include "shapes.h"
#include "tile_ids.h"
#include "ecs_pickups.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_lifetime.h"
#include "graphics_globals.h"

namespace ecs {
	const float _PLAYER_WALK_SPEED = 60.f;
	const float _PLAYER_RUN_SPEED = 136.f;
	const float _PLAYER_STEALTH_SPEED = 36.f;
	const float _PLAYER_ARROW_SPEED = 160.f;

	extern entt::registry _registry;
	unsigned int _input_flags_to_enable = 0;
	unsigned int _input_flags_to_disable = 0;

	void handle_window_event_for_players(const window::Event& ev) {
		if (ev.type == window::EventType::KeyPress) {
			switch (ev.key.code) {
			case window::Key::Left:
				_input_flags_to_enable |= INPUT_LEFT;
				break;
			case window::Key::Right:
				_input_flags_to_enable |= INPUT_RIGHT;
				break;
			case window::Key::Up:
				_input_flags_to_enable |= INPUT_UP;
				break;
			case window::Key::Down:
				_input_flags_to_enable |= INPUT_DOWN;
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
				_input_flags_to_disable |= INPUT_LEFT;
				break;
			case window::Key::Right:
				_input_flags_to_disable |= INPUT_RIGHT;
				break;
			case window::Key::Up:
				_input_flags_to_disable |= INPUT_UP;
				break;
			case window::Key::Down:
				_input_flags_to_disable |= INPUT_DOWN;
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

	void _player_attack(entt::entity entity, const Vector2f& position) {
		Vector2f box_min = position - Vector2f(6.f, 6.f);
		Vector2f box_max = position + Vector2f(6.f, 6.f);
		apply_damage_in_box({ DamageType::Melee, 1, entity }, box_min, box_max, ~CC_Player);
	}

	void setup_players() {
		for (auto [entity, object] : _registry.view<Type<Tag::Player>, TObject>().each()) {
			const Vector2f top_left = object.get_top_left();

			Player player{};
			/*if (last_player) { // TODO
				player = *last_player;
				player.input_flags = 0;
			}*/

			const Vector2f pivot = { 32.f, 42.f };

			{
				player::Outfit outfit{};
				player::randomize_outfit(outfit);
				player::create_outfit_texture(outfit);
			}

			if (sprites::Sprite* sprite = get_sprite(entity)) {
				sprite->texture = graphics::get_framebuffer_texture(graphics::player_outfit_framebuffer);
				sprite->sorting_point = pivot;
			}
			{
				b2BodyDef body_def = b2DefaultBodyDef();
				body_def.type = b2_dynamicBody;
				body_def.position = top_left + pivot;
				body_def.fixedRotation = true;
				b2BodyId body = emplace_body(entity, body_def);
				b2ShapeDef shape_def = b2DefaultShapeDef();
				shape_def.filter = get_physics_filter_for_tag(Tag::Player);
				b2Circle circle{};
				circle.radius = 7.f;
				b2CreateCircleShape(body, &shape_def, &circle);
			}

			make_sprite_follow_body(entity, -pivot);

			/*if (last_active_portal) { // TODO

				if (const tiled::Object* target_point = tiled::find_object_with_name(map, last_active_portal->target_point)) {
#if 0
					if (body) {
						body->SetTransform(target_point->position_top_left, 0.f);
					}
#endif
				}

				if (last_active_portal->exit_direction == "up") {
					player.look_dir = { 0.f, -1.f };
				} else if (last_active_portal->exit_direction == "down") {
					player.look_dir = { 0.f, 1.f };
				} else if (last_active_portal->exit_direction == "left") {
					player.look_dir = { -1.f, 0.f };
				} else if (last_active_portal->exit_direction == "right") {
					player.look_dir = { 1.f, 0.f };
				}
			}*/

			player.held_item = _registry.create();
			emplace_tile_animation(player.held_item);
			emplace_player(entity, player);

			set_physics_event_handler(entity, on_player_physics_event);
			set_damage_handler(entity, apply_damage_to_player);

			Camera camera{};
			camera.center = top_left;
			camera.confines_min = { 0.f, 0.f };
			camera.confines_max = object.get_map().get_bottom_right();
			camera.entity_to_follow = entity;
			emplace_camera(entity, camera);
			activate_camera(entity, true);
		}
	}

	void update_players(float dt) {
		const bool player_accepts_input = (dt > 0.f && window::has_focus() && !console::has_focus());

		for (auto [player_entity, player, body, sprite, animation] :
			_registry.view<Player, b2BodyId, sprites::Sprite, TileAnimation>().each()) {

			if (player_accepts_input) {
				player.input_flags |= _input_flags_to_enable;
				player.input_flags &= ~_input_flags_to_disable;
			} else {
				player.input_flags = 0;
			}

			// UPDATE TIMERS

			player.hurt_timer.update(dt);

			// GET PHYSICS STATE

			const Vector2f position = b2Body_GetPosition(body);
			const Vector2f velocity = b2Body_GetLinearVelocity(body);
			Vector2f new_velocity; // will be modified differently depending on the state

			enum class HeldItemType {
				None,
				Sword,
				Bow,
			};

			HeldItemType held_item_type = HeldItemType::None;

			// UPDATE AUDIO

			audio::set_listener_position(position);
			audio::set_parameter_label("terrain", map::to_string(map::get_terrain_type_at(position)));
			if (animation._dirty && animation._frame_id % 3 == 0) {
				// Take a step every 3 frames
				audio::create_event({ .path = "event:/snd_footstep" });
			}

			// UPDATE POST-PROCESSING
			postprocessing::set_darkness_center(position);

			const char dir = get_direction(player.look_dir);

			switch (dir) {
			case 'l': sprite.flags |= sprites::SPRITE_FLIP_HORIZONTALLY; break;
			case 'r': sprite.flags &= ~sprites::SPRITE_FLIP_HORIZONTALLY; break;
			}

			switch (player.state) {
			case PlayerState::Normal: {

				Vector2f new_move_dir;
				float new_move_speed = 0.f;

				if (player.input_flags & INPUT_LEFT)  new_move_dir.x--;
				if (player.input_flags & INPUT_RIGHT) new_move_dir.x++;
				if (player.input_flags & INPUT_UP)    new_move_dir.y--;
				if (player.input_flags & INPUT_DOWN)  new_move_dir.y++;

				if (!is_zero(new_move_dir)) {
					player.look_dir = new_move_dir;
					new_move_dir = normalize(new_move_dir);
					if (player.input_flags & INPUT_STEALTH) {
						new_move_speed = _PLAYER_STEALTH_SPEED;
					} else if (player.touching_pushable_block) {
						new_move_speed = _PLAYER_WALK_SPEED;
					} else if (player.input_flags & INPUT_RUN) {
						new_move_speed = _PLAYER_RUN_SPEED;
					} else {
						new_move_speed = _PLAYER_WALK_SPEED;
					}
				}

				new_velocity = new_move_dir * new_move_speed;

				if (player.input_flags & INPUT_SWING_SWORD) {

					switch (dir) {
					case 'l':
					case 'r': animation.tile_id = TILE_ID_PLAYER_FOREHAND_STRIKE_RIGHT; break;
					case 'u': animation.tile_id = TILE_ID_PLAYER_FOREHAND_STRIKE_UP; break;
					case 'd': animation.tile_id = TILE_ID_PLAYER_FOREHAND_STRIKE_DOWN; break;
					}

					audio::create_event({ .path = "event:/snd_sword_attack" });
					animation.progress = 0.f;
					animation.loop = false;

					player.state = PlayerState::SwingingSword;

				} else if (player.input_flags & INPUT_SHOOT_BOW && player.arrows > 0) {

					switch (dir) {
					case 'l':
					case 'r': animation.tile_id = TILE_ID_PLAYER_BOW_SHOT_RIGHT; break;
					case 'u': animation.tile_id = TILE_ID_PLAYER_BOW_SHOT_UP; break;
					case 'd': animation.tile_id = TILE_ID_PLAYER_BOW_SHOT_DOWN; break;
					}

					animation.progress = 0.f;
					animation.loop = false;

					player.state = PlayerState::ShootingBow;

				} else if (player.input_flags & INPUT_DROP_BOMB && player.bombs > 0) {

					const Vector2f bomb_position = position + player.look_dir * 16.f;
					if (create_bomb(bomb_position) != entt::null) {
						player.bombs--;
						audio::create_event({ .path = "event:/player/place_bomb" });
					} else {
						audio::create_event({ .path = "event:/player/error" });
					}

				} else if (player.touching_pushable_block && !is_zero(new_velocity)) {

					switch (dir) {
					case 'l':
					case 'r': animation.tile_id = TILE_ID_PLAYER_PUSH_RIGHT; break;
					case 'u': animation.tile_id = TILE_ID_PLAYER_PUSH_UP; break;
					case 'd': animation.tile_id = TILE_ID_PLAYER_PUSH_DOWN; break;
					}

					animation.loop = true;
					if (animation._looped && (dir == 'u' || dir == 'd')) {
						sprite.flags ^= sprites::SPRITE_FLIP_HORIZONTALLY;
					}

				} else if (new_move_speed >= _PLAYER_RUN_SPEED) {

					switch (dir) {
					case 'l':
					case 'r': animation.tile_id = TILE_ID_PLAYER_RUN_RIGHT; break;
					case 'u': animation.tile_id = TILE_ID_PLAYER_RUN_UP; break;
					case 'd': animation.tile_id = TILE_ID_PLAYER_RUN_DOWN; break;
					}

					animation.loop = true;
					if (animation._looped && (dir == 'u' || dir == 'd')) {
						sprite.flags ^= sprites::SPRITE_FLIP_HORIZONTALLY;
					}

				} else if (new_move_speed >= _PLAYER_WALK_SPEED) {

					switch (dir) {
					case 'l':
					case 'r': animation.tile_id = TILE_ID_PLAYER_WALK_RIGHT; break;
					case 'u': animation.tile_id = TILE_ID_PLAYER_WALK_UP; break;
					case 'd': animation.tile_id = TILE_ID_PLAYER_WALK_DOWN; break;
					}

					animation.loop = true;
					if (animation._looped && (dir == 'u' || dir == 'd')) {
						sprite.flags ^= sprites::SPRITE_FLIP_HORIZONTALLY;
					}

				} else {

					switch (dir) {
					case 'l':
					case 'r': animation.tile_id = TILE_ID_PLAYER_IDLE_RIGHT; break;
					case 'u': animation.tile_id = TILE_ID_PLAYER_IDLE_UP; break;
					case 'd': animation.tile_id = TILE_ID_PLAYER_IDLE_DOWN; break;
					}

					animation.loop = true;
					sprite.flags &= ~sprites::SPRITE_FLIP_VERTICALLY;

				}

				if (player.input_flags & INPUT_INTERACT) {
					const Vector2f box_center = position + player.look_dir * 16.f;
					const Vector2f box_min = box_center - Vector2f(6.f, 6.f);
					const Vector2f box_max = box_center + Vector2f(6.f, 6.f);
					interact_with_all_entities_in_box(box_min, box_max);
				}

			} break;
			case PlayerState::SwingingSword: {
				held_item_type = HeldItemType::Sword;
#if 0
				if (tile_dir != 'r') {
					sprite.flags &= ~sprites::SPRITE_FLIP_HORIZONTALLY;
				}
#endif
				if (animation._dirty && animation._frame_id == 1) {
					_player_attack(player_entity, position + player.look_dir * 16.f);
				}
				if (animation.progress == 1.f) {
					player.state = PlayerState::Normal;
				}
			} break;
			case PlayerState::ShootingBow: {
				held_item_type = HeldItemType::Bow;
#if 0
				if (tile_dir != 'r') {
					sprite.flags &= ~sprites::SPRITE_FLIP_HORIZONTALLY;
				}
#endif
				if (player.arrows > 0 && animation._dirty && animation._frame_id == 2) {
					player.arrows--;
					create_arrow(position + player.look_dir * 16.f, player.look_dir * _PLAYER_ARROW_SPEED);
				}
				if (animation.progress == 1.f) {
					player.state = PlayerState::Normal;
				}
			} break;
			case PlayerState::Dying: {

				unsigned int original_tile_id = animation.tile_id;

				switch (dir) {
				case 'l':
				case 'r': animation.tile_id = TILE_ID_PLAYER_DYING_RIGHT_DOWN; break;
				case 'u': animation.tile_id = TILE_ID_PLAYER_DYING_RIGHT_UP; break;
				case 'd': animation.tile_id = TILE_ID_PLAYER_DYING_RIGHT_DOWN; break;
				}

				if (animation.tile_id != original_tile_id) { // HACK
					animation.progress = 0.f;
				}
				animation.loop = false;

				if (animation.progress == 1.f) {

					switch (dir) {
					case 'l':
					case 'r': animation.tile_id = TILE_ID_PLAYER_DEAD_RIGHT_DOWN; break;
					case 'u': animation.tile_id = TILE_ID_PLAYER_DEAD_RIGHT_UP; break;
					case 'd': animation.tile_id = TILE_ID_PLAYER_DEAD_RIGHT_DOWN; break;
					}

					kill_player(player_entity);
					player.state = PlayerState::Dead;
				}

			} break;
			case PlayerState::Dead: {
				// Do nothing, u r ded
			} break;
			}

			b2Body_SetLinearVelocity(body, new_velocity);

			// UPDATE HELD ITEM GRAPHICS

#if 0
			if (TileAnimation* held_item_tile = get_tile_animation(player.held_item)) {
				held_item_tile->set_flag(TILE_VISIBLE, held_item_type != HeldItemType::None);
				Vector2f player_tile_sorting_pos = animation.offset - animation.pivot + animation.sorting_pivot;
				switch (held_item_type) {
				case HeldItemType::Sword: {
					held_item_tile->set_tileset("sword");
					held_item_tile->set_tile(3);
					held_item_tile->offset = offset;
					held_item_tile->offset.y -= 10.f;
					held_item_tile->pivot = { 16.f, 16.f };
					held_item_tile->sorting_pivot =
						player_tile_sorting_pos - held_item_tile->offset + held_item_tile->pivot;
					switch (dir) {
					case 'u':
						held_item_tile->set_rotation((int)animation._frame_id - 1);
						switch (animation._frame_id) {
						case 0: held_item_tile->offset += { -18.f, -3.f }; break;
						case 1: held_item_tile->offset += { 0.f, -21.f }; break;
						case 2: held_item_tile->offset += { 20.f, -1.f }; break;
						}
						break;
					case 'r':
						held_item_tile->set_rotation(2 - (int)animation._frame_id);
						switch (animation._frame_id) {
						case 0: held_item_tile->offset += { -0.f, 18.f }; break;
						case 1: held_item_tile->offset += { 28.f, 0.f }; break;
						case 2: held_item_tile->offset += { 1.f, -18.f }; break;
						}
						break;
					case 'd':
						held_item_tile->set_rotation((int)animation._frame_id + 1);
						switch (animation._frame_id) {
						case 0: held_item_tile->offset += { 18.f, 2.f }; break;
						case 1: held_item_tile->offset += { 0.f, 20.f }; break;
						case 2: held_item_tile->offset += { -18.f, -2.f }; break;
						}
						break;
					case 'l':
						held_item_tile->set_rotation((int)animation._frame_id + 2);
						switch (animation._frame_id) {
						case 0: held_item_tile->offset += { 2.f, 18.f }; break;
						case 1: held_item_tile->offset += { -28.f, 0.f }; break;
						case 2: held_item_tile->offset += { -2.f, -18.f }; break;
						}
						break;
					}
				} break;
				case HeldItemType::Bow: {
					held_item_tile->set_tileset("bow_01");
					held_item_tile->set_tile(animation._frame_id);
					held_item_tile->offset = offset;
					held_item_tile->offset.y -= 13.f;
					held_item_tile->pivot = { 16.f, 16.f };
					held_item_tile->sorting_pivot =
						player_tile_sorting_pos - held_item_tile->offset + held_item_tile->pivot;
					switch (dir) {
					case 'r':
						held_item_tile->set_rotation(0);
						held_item_tile->offset.x += 16.f;
						held_item_tile->offset.y += 2.f;
						held_item_tile->sorting_pivot.y -= 2.f;
						held_item_tile->sorting_pivot.y += 1.f;
						break;
					case 'd':
						held_item_tile->set_rotation(1);
						held_item_tile->offset.y += 11.f;
						held_item_tile->sorting_pivot.y -= 11.f;
						held_item_tile->sorting_pivot.y += 1.f;
						break;
					case 'l':
						held_item_tile->set_rotation(2);
						held_item_tile->offset.x -= 16.f;
						held_item_tile->offset.y += 3.f;
						held_item_tile->sorting_pivot.y -= 3.f;
						held_item_tile->sorting_pivot.y += 1.f;
						break;
					case 'u':
						held_item_tile->set_rotation(3);
						held_item_tile->offset.y -= 11.f;
						held_item_tile->sorting_pivot.y += 11.f;
						held_item_tile->sorting_pivot.y -= 1.f;
						break;
					}
				} break;
				}
			}
#endif

			// UPDATE SPRITE COLOR

			if (player.hurt_timer.running()) {
				constexpr float BLINK_PERIOD = 0.15f;
				float fraction = fmod(player.hurt_timer.get_time(), BLINK_PERIOD) / BLINK_PERIOD;
				sprite.color.a = (unsigned char)(255 * fraction);
			} else {
				sprite.color.a = 255;
			}

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

			Vector2f position = b2Body_GetPosition(body);
			ImGui::Text("Position: %.1f, %.1f", position.x, position.y);
			ImGui::Text("Terrain: %s", map::to_string(map::get_terrain_type_at(position)).c_str());
			ImGui::Text("State: %s", magic_enum::enum_name(player.state).data());
			ImGui::Text("Health: %d", player.health);
			ImGui::Text("Arrows: %d", player.arrows);
			ImGui::Text("Rupees: %d", player.rupees);

			if (ImGui::Button("Apply 1 Damage")) {
				apply_damage_to_player(entity, { DamageType::Default, 1 });
			}
			if (ImGui::Button("Kill")) {
				apply_damage_to_player(entity, { DamageType::Default, 999 });
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

	entt::entity find_player_entity() {
		return _registry.view<Player>().front();
	}

	Player& emplace_player(entt::entity entity, const Player& player) {
		return _registry.emplace_or_replace<Player>(entity, player);
	}

	Player* get_player(entt::entity entity) {
		return _registry.try_get<Player>(entity);
	}

	bool remove_player(entt::entity entity) {
		return _registry.remove<Player>(entity);
	}

	bool has_player(entt::entity entity) {
		return _registry.all_of<Player>(entity);
	}

	bool kill_player(entt::entity entity) {
		if (!_registry.all_of<Player>(entity)) return false;
		detach_camera(entity);
		audio::stop_all_in_bus();
		audio::create_event({ .path = "event:/snd_player_die" });
		audio::create_event({ .path = "event:/mus_coffin_dance" });
		ui::open_or_enqueue_textbox_presets("player/die");
		ui::bindings::hud_player_health = 0;
		return true;
	}

	void _on_player_begin_touch_pickup(entt::entity player_entity, entt::entity pickup_entity) {
		Player* player = get_player(player_entity);
		if (!player) return;
		Pickup* pickup = get_pickup(pickup_entity);
		if (!pickup) return;

		switch (pickup->type) {
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

		destroy_at_end_of_frame(pickup_entity);
	}

	void _on_player_begin_touch_pushable_block(entt::entity player_entity, entt::entity pushable_block_entity) {
		Player* player = get_player(player_entity);
		if (!player) return;
		player->touching_pushable_block = true;
		audio::stop_event(player->stone_sliding_sound); // Stop any previously playing sound
		player->stone_sliding_sound = audio::create_event({ .path = "event:/props/stone_slide" });
		if (b2BodyId body = get_body(pushable_block_entity); B2_IS_NON_NULL(body)) {
			b2Body_SetType(body, b2_dynamicBody);
		}
	}

	void _on_player_end_touch_pushable_block(entt::entity player_entity, entt::entity pushable_block_entity) {
		Player* player = get_player(player_entity);
		if (!player) return;
		player->touching_pushable_block = false;
		audio::stop_event(player->stone_sliding_sound);
		if (b2BodyId body = get_body(pushable_block_entity); B2_IS_NON_NULL(body)) {
			b2Body_SetType(body, b2_staticBody);
		}
	}

	void on_player_physics_event(const PhysicsEvent& ev) {
		if (ev.type == PhysicsEventType::SensorBeginTouch) {
			if (ev.tag_b == Tag::Pickup) {
				_on_player_begin_touch_pickup(ev.entity_a, ev.entity_b);
			} else if (ev.tag_b == Tag::Portal) {
				activate_portal(ev.entity_b);
			}
		} else if (ev.type == PhysicsEventType::ContactBeginTouch) {
			if (ev.tag_b == Tag::PushableBlock) {
				_on_player_begin_touch_pushable_block(ev.entity_a, ev.entity_b);
			} else if (ev.tag_b == Tag::Slime) {
				apply_damage_to_player(ev.entity_a, { DamageType::Melee, 1 });
			} 
		} else if (ev.type == PhysicsEventType::ContactEndTouch) {
			if (ev.tag_b == Tag::PushableBlock) {
				_on_player_end_touch_pushable_block(ev.entity_a, ev.entity_b);
			}
		}
	}

	bool apply_damage_to_player(entt::entity entity, const Damage& damage) {
		if (damage.amount <= 0) return false;
		Player* player = get_player(entity);
		if (!player) return false;
		if (player->health <= 0) return false; // Player is already dead
		if (player->hurt_timer.running()) return false; // Player is invulnerable
		player->health = std::max(0, player->health - damage.amount);
		add_trauma_to_active_camera(0.8f);
		if (player->health > 0) { // Player survived
			audio::create_event({ .path = "event:/snd_player_hurt" });
			player->hurt_timer.start();
		} else { // Player died
			player->state = PlayerState::Dying;
		}
		return true;
	}
}
