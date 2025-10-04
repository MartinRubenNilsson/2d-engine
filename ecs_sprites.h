#pragma once
#include "sprites.h"

namespace ecs {
	sprites::Sprite& emplace_sprite(entt::entity entity);
	sprites::Sprite* get_sprite(entt::entity entity);

	void update_sprite(sprites::Sprite& sprite, TileId tile);

	void make_sprite_follow_body(entt::entity entity, const Vector2f& offset = { 0.f, 0.f });

	// Makes the Sprite's color blink.
	struct SpriteBlink {
		float duration = 0.f; // in seconds
		float interval = 0.f; // in seconds
		Color color = colors::WHITE;
		Color _original_color; // for internal use only!
	};

	void make_sprite_blink(entt::entity entity, SpriteBlink&& blink = SpriteBlink());

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

	void make_sprite_shake(entt::entity entity, SpriteShake&& shake = SpriteShake());

	void setup_sprites(MapId map);
	void update_sprites(float dt);
	void draw_sprites(const Vector2f& camera_min, const Vector2f& camera_max);
}