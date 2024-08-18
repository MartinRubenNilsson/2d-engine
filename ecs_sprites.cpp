#include "stdafx.h"
#include "ecs_sprites.h"
#include "random.h"

namespace ecs
{
	extern entt::registry _registry;

	void update_sprites_following_bodies()
	{
		for (auto [entity, sprite, body, follow]  :
			_registry.view<sprites::Sprite, b2BodyId, SpriteFollowBody>().each()) {
			sprite.position = b2Body_GetPosition(body) + follow.offset;
		}
	}

	void update_sprite_shakes(float dt)
	{
		for (auto [entity, shake] : _registry.view<SpriteShake>().each()) {
			const float last_duration = shake.duration;
			if (shake.duration > 0.f) {
				shake.duration -= dt;
			}
			if (shake.duration <= 0.f) {
				_registry.erase<SpriteShake>(entity);
				continue;
			}
			shake.magnitude *= pow(shake.duration / last_duration, std::max(shake.exponent, 0.f));
		}
	}

	void shake_sprites_before_drawing()
	{
		for (auto [entity, sprites, shake] : _registry.view<sprites::Sprite, SpriteShake>().each()) {
			shake._original_position = sprites.position;
			sprites.position.x += shake.magnitude * 
				random::fractal_perlin_noise(10.f * shake.duration, 0.f, 0.f);
			sprites.position.y += shake.magnitude *
				random::fractal_perlin_noise(10.f * shake.duration, 10.f, 0.f);
		}
	}

	void unshake_sprites_after_drawing()
	{
		for (auto [entity, sprites, shake] : _registry.view<sprites::Sprite, SpriteShake>().each()) {
			sprites.position = shake._original_position;
		}
	}

	sprites::Sprite& emplace_sprite(entt::entity entity)
	{
		return _registry.emplace_or_replace<sprites::Sprite>(entity);
	}

	sprites::Sprite* get_sprite(entt::entity entity)
	{
		return _registry.try_get<sprites::Sprite>(entity);
	}

	SpriteFollowBody& emplace_sprite_follow_body(entt::entity entity, const Vector2f& offset)
	{
		return _registry.emplace_or_replace<SpriteFollowBody>(entity, offset);
	}

	SpriteFollowBody* get_sprite_follow_body(entt::entity entity)
	{
		return _registry.try_get<SpriteFollowBody>(entity);
	}

	SpriteShake& emplace_sprite_shake(entt::entity entity)
	{
		return _registry.emplace_or_replace<SpriteShake>(entity);
	}
}