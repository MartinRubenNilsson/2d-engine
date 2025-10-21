#pragma once

namespace ecs {
	class TileAnimation {
	public:
		// Returns the index of the current frame in the animation.
		unsigned int get_frame() const;
		// Returns the tile of the current frame in the animation.
		TileId get_tile() const;
		// Sets the animation progress (aka percentage or normalized time). Can be in the range [0, 1].
		void set_progress(float progress);
		// Sets the animation progress (aka percentage or normalized time). Can be in the range [0, 1].
		float get_progress() const;
		// Returns  the playback speed multiplier. 1.f corresponds to normal speed.
		void set_speed(float speed);
		// Returns the playback speed multiplier. 1.f corresponds to normal speed.
		float get_speed() const;
		// Sets whether the animation should loop.
		void set_loop(bool loop);
		// Returns whether the anmation is looping.
		bool get_loop() const;
		// Returns true if the animation is non-looping and its progress is at 1.f.
		bool done() const; 
		// Returns true if the animation looped during its last update.
		bool looped() const;
		// Returns true if the frame index changed during the last update.
		bool frame_changed() const;
		// Returns true if the frame tile changed during the last update. PITFALL: This may happen even
		// if the frame didn't change, as for example if any flip flags change.
		bool tile_changed() const;

	private:
		unsigned int _frame = UINT_MAX; // The current frame index in the animation.
		TileId _tile{}; // The tile of the current frame.
		float _progress = 0.f; 
		float _speed = 1.f; // Playback speed multiplier.
		bool _loop = true; // Should the animation loop?
		bool _looped = false; 
		bool _frame_changed = false;
		bool _tile_changed = false;

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
