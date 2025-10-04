#pragma once
#include "ecs_tiled.h"

namespace ecs {
	struct TileAnimation {
		TileId tile{}; // The tile that owns the animation.
		TileId _frame{}; // The current frame in the animation.
		float progress = 0.f; // aka normalized time, in the range [0, 1]
		float speed = 1.f;
		bool loop = true;
		bool _looped = false;
		bool _frame_changed = false;
	};

	struct FlipbookAnimation {
		unsigned int rows = 0;
		unsigned int columns = 0;
		float fps = 0.f;
		float time = 0.f;
	};

	void setup_tile_animations();
	void update_tile_animations(float dt);
	void update_flipbook_animations(float dt);
	void update_animated_sprites(float dt);

	TileAnimation& emplace_tile_animation(entt::entity entity);
	TileAnimation* get_tile_animation(entt::entity entity);

	FlipbookAnimation& emplace_flipbook_animation(entt::entity entity);
}
