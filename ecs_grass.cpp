#include "stdafx.h"
#include "ecs_grass.h"
#include "ecs_common.h"
#include "ecs_physics.h"
#include "ecs_pickups.h"
#include "ecs_uniform_block.h"
#include "random.h"
#include "audio.h"
#include "sprites.h"
#include "ecs_damage.h"
#include "graphics_globals.h"

namespace ecs {
	extern entt::registry _registry;

	bool _handle_damage_to_grass(entt::entity entity, const Damage& damage) {
		b2BodyId body = get_body(entity);
		if (B2_IS_NULL(body)) return false;
		const Vector2f position = b2Body_GetPosition(body);
		audio::create_event({ .path = "event:/snd_cut_grass" });
		if (random::chance(0.2f)) {
			PickupType pickup_type = (PickupType)random::range_i(0, (int)PickupType::Count - 1);
			create_pickup(pickup_type, position + Vector2f(8.f, 20.f));
		}
		destroy_at_end_of_frame(entity);
		return true;
	}

	struct GrassUniformBlock {
		Vector2f position;
		Vector2f tex_min;
		Vector2f tex_max;
	};

	void setup_grass() {
		for (auto [entity, sprite] : _registry.view<Type<Tag::Grass>, sprites::Sprite>().each()) {
			sprite.vertex_shader = graphics::grass_vert;
			sprite.sorting_point = { 8.f, 28.f };
			{
				GrassUniformBlock block{};
				block.position = sprite.position;
				block.tex_min = sprite.tex_position;
				block.tex_max = sprite.tex_position + sprite.tex_size;
				ecs::emplace_uniform_block(entity, &block, sizeof(ecs::GrassUniformBlock));
			}
			ecs::set_damage_handler(entity, ecs::_handle_damage_to_grass);
		}
	}
}
