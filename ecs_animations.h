#pragma once

namespace ecs {
	class TileAnimation {
	public:
		void set_progress(float progress);
		float get_progress() const;
		void set_speed(float speed);
		void set_loop(bool loop);

		bool done() const; // check if non-looping and progress = 1
		bool looped() const; // check if the animation looped last update

	private:
		TileId _frame{}; // The current frame in the animation.
		float _progress = 0.f; // aka normalized time, in the range [0, 1]
		float _speed = 1.f;
		bool _loop = true; // Should the animation loop?
		bool _looped = false;
		bool _frame_changed = false;

		friend void _update_tile_animations(float dt);
		friend void _update_tile_animated_sprites(float dt);
	};

	TileAnimation& emplace_tile_animation(entt::entity entity);

	struct FlipbookAnimation {
		unsigned int rows = 0;
		unsigned int columns = 0;
		float fps = 0.f;
		float time = 0.f;
	};

	FlipbookAnimation& emplace_flipbook_animation(entt::entity entity);

	void setup_animations();
	void update_animations(float dt);
	void update_animated_sprites(float dt);
}
