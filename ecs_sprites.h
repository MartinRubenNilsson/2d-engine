#pragma once
#include "sprites.h"

namespace ecs {

	// Makes the Sprite follow along a b2BodyId as the latter moves.
	struct SpriteFollowBody {
		Vector2f offset; // the sprite's position relative to the body's position
	};

	// Makes the Sprite's color blink.
	struct SpriteBlink {
		float duration = 0.f; // in seconds
		float interval = 0.f; // in seconds
		Color color = colors::WHITE;
		Color _original_color; // for internal use only!
	};

	// Makes the Sprite shake.
	struct SpriteShake {
		// The duration decreases by dt each frame, while the magnitude decreases
		// in such a way that the two values always satisfy the following power law:
		// 
		//     magnitude = initial_magnitude * pow(duration / initial_duration, exponent).
		//
		// The magnitude remains constant when exponent = 0, decreases linearly when exponent = 1,
		// decreases faster at the end when 0 < exponent < 1, and decreases faster initially
		// when exponent > 1. A natural choice for a shake that fades out is exponent = 2.

		float duration = 0.f; // in seconds
		float magnitude = 0.f; // in pixels
		float exponent = 0.f;
		unsigned int _random_seed = 0; // auto-generated at emplace time
		Vector2f _original_position; // for internal use only!
	};

	void update_sprites_following_bodies();
	void update_sprite_blinks(float dt);
	void update_sprite_shakes(float dt);

	void blink_sprites_before_drawing();
	void unblink_sprites_after_drawing();
	void shake_sprites_before_drawing();
	void unshake_sprites_after_drawing();

	sprites::Sprite& emplace_sprite(entt::entity entity);
	sprites::Sprite* get_sprite(entt::entity entity);

	SpriteFollowBody& emplace_sprite_follow_body(entt::entity entity, const Vector2f& offset = { 0.f, 0.f });
	SpriteFollowBody* get_sprite_follow_body(entt::entity entity);

	SpriteBlink& emplace_sprite_blink(entt::entity entity);

	void emplace_sprite_shake(entt::entity entity, SpriteShake&& shake = SpriteShake());
}