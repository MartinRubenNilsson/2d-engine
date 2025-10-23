#include "stdafx.h"
#include "ecs_grass.h"
#include "ecs_lifetime.h"
#include "ecs_physics.h"
#include "ecs_pickups.h"
#include "ecs_uniform_block.h"
#include "ecs_tags.h"
#include "random.h"
#include "audio.h"
#include "sprites.h"
#include "ecs_damage.h"
#include "graphics_globals.h"

namespace ecs {
	extern entt::registry _registry;

	bool _grass_handle_damage(entt::entity entity, const DamageEvent& ev) {
		b2BodyId body = get_body(entity);
		if (B2_IS_NULL(body)) return false;
		const Vec2f position = b2Body_GetWorldCenterOfMass(body);
		audio::create_event("event:/snd_cut_grass");
		if (random::boolean(0.2f)) {
			PickupType pickup_type = (PickupType)random::uniform_int(0, (int)PickupType::Count - 1);
			create_pickup(pickup_type, position + Vec2f(8.f, 20.f));
		}
		destroy_later(entity);
		return true;
	}

	struct GrassUniformBlock {
		Vec2f position;
		Vec2f tex_min;
		Vec2f tex_max;
	};

	Handle<graphics::VertexShader> _grass_vert{};

	void startup_grass() {
		_grass_vert = graphics::load_vertex_shader("assets/shaders/grass.vert");
	}

	void setup_grass() {
		for (auto [entity, sprite] : _registry.view<Type<Tag::Grass>, sprites::Sprite>().each()) {
			sprite.vertex_shader = _grass_vert;
			{
				GrassUniformBlock block{};
				block.position = sprite.position;
				block.tex_min = sprite.tex_position;
				block.tex_max = sprite.tex_position + sprite.tex_size;
				emplace_uniform_block(entity, &block, sizeof(GrassUniformBlock));
			}
			set_damage_event_handler(entity, _grass_handle_damage);
		}
	}
}
