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
#include "console.h"
#include "map.h"

namespace ecs {

	void replace(TileId& tile, Direction dir, int east_tile_id, int north_tile_id, int south_tile_id) {
		switch (dir) {
			case Direction::W: [[fallthrough]];
			case Direction::E: replace(tile, east_tile_id); break;
			case Direction::N: replace(tile, north_tile_id); break;
			case Direction::S: replace(tile, south_tile_id); break;
		}
		if (dir == Direction::E) {
			tile.flipped_horizontally = false; // Never flip if we're facing east (right).
		} else if (dir == Direction::W) {
			tile.flipped_horizontally = true;  // Always flip if we're facing west (left).
		}
	}

	extern entt::registry _registry;

	void _player_handle_touch_bounds(const Vec2f& position, const Vec2f& normal) {
#if 0 // Enable to debug print.
		console::log(std::format("({0}, {1}), ({2}, {3})", position.x, position.y, normal.x, normal.y));
#endif
		const WorldId curr_world = get_world("assets/tiled/maps/forest/forest.world"); // HACK
		if (!curr_world) return;
		const MapId curr_map = map::get_current_map();
		if (!curr_map) return;
		const Vec2i curr_map_world_pos = get_position_of_map(curr_world, curr_map);
		const Vec2i player_curr_world_pos = Vec2i(position) + curr_map_world_pos;
		const Vec2i player_next_world_pos = player_curr_world_pos + Vec2i(normal) * 16;
		const MapId next_map = get_map_at_position(curr_world, player_next_world_pos);
		if (!next_map) return;
		const Vec2i next_map_world_pos = get_position_of_map(curr_world, next_map);
		Patch& patch = get_patch(next_map);
		patch.player_position = player_next_world_pos - next_map_world_pos;
		map::open(get_path(next_map));
	}

	void _player_handle_touch_pickup(entt::entity player_entity, entt::entity pickup_entity) {
		Player* player = _registry.try_get<Player>(player_entity);
		if (!player) return;

		const PickupType type = get_pickup_type(pickup_entity);
		switch (type) {
			case PickupType::Arrow: {
				player->arrows++;
				audio::create_event("event:/snd_pickup");
			} break;
			case PickupType::Rupee: {
				player->rupees++;
				audio::create_event("event:/snd_pickup_rupee");
			} break;
			case PickupType::Bomb: {
				player->bombs++;
				audio::create_event("event:/snd_pickup");
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
		const Tag other_tag = get_tag(ev.other_entity);
		if (ev.type == TouchEventType::ContactBegin) {
			if (other_tag == Tag::Bounds) {
				_player_handle_touch_bounds(ev.manifold.points[0].point, ev.manifold.normal);
			}
		} else if (ev.type == TouchEventType::SensorBegin) {
			if (other_tag == Tag::Pickup) {
				_player_handle_touch_pickup(ev.entity, ev.other_entity);
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

	void _teleport_players_to_position(const Vec2f& position) {
		for (auto [entity, body] : _registry.view<Type<Tag::Player>, b2BodyId>().each()) {
			b2Body_SetTransform(body, position - b2Body_GetLocalCenterOfMass(body), b2Rot_identity);
		}
	}

	void patch_players(const Patch& patch) {
		if (!patch.portal_to_exit.empty()) {
			_teleport_players_to_portal(patch.portal_to_exit);
		} else if (patch.player_position != Vec2f::MAX) {
			_teleport_players_to_position(patch.player_position);
		}
	}

	void update_players(float dt) {
		// Update hud.
		for (auto [entity, player] : _registry.view<Player>().each()) {
			ui::hud::health = player.health;
			ui::hud::arrows = player.arrows;
			ui::hud::bombs = player.bombs;
			ui::hud::rupees = player.rupees;
		}
	}
}
