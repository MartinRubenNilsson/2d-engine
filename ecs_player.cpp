#include "stdafx.h"
#include "ecs_player.h"
#include "ecs_player_types.h"
#include "ecs_physics_events.h"
#include "ecs_sprites.h"
#include "ecs_animations.h"
#include "ecs_camera.h"
#include "ecs_damage.h"
#include "ecs_pickups.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "ecs_portal.h"
#include "ecs_lifetime.h"
#include "ecs_patch.h"
#include "ecs_states.h"
#include "ecs_audio.h"
#include "ecs_player_states.h"
#include "graphics_globals.h"
#include "player_outfit.h"
#include "audio.h"
#include "ui_hud.h"

namespace ecs {

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

	void _player_handle_touch(const TouchEvent& ev) {
		if (ev.type == TouchEventType::SensorBegin) {
			const Tag other_tag = get_tag(ev.other_entity);
			if (other_tag == Tag::Pickup) {
				_player_begin_touch_pickup(ev.entity, ev.other_entity);
			}
		}
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

	void _emplace_player_state_machine(entt::entity entity) {
		StateMachine& sm = emplace_state_machine(entity);
		StateId alive = add_player_alive_state(sm);
			StateId normal = add_player_normal_state(sm, alive);
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

	void update_players(float dt) {

		// Update hud. TODO: put in ecs_ui_hud.h or something
		for (auto [entity, player] : _registry.view<Player>().each()) {
			ui::hud::health = player.health;
			ui::hud::arrows = player.arrows;
			ui::hud::bombs = player.bombs;
			ui::hud::rupees = player.rupees;
		}
	}
}
