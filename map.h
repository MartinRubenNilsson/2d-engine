#pragma once

namespace map {
	bool has_current_map();
	ecs::MapId get_current_map();

	// A callback that is called the precise moment the map changes, which is when we have
	// transitioned out completely (so that get_transition_progress() == 1.f) and is just
	// about to start transitioning into the new map (if any).
	using TransitionCallback = void(*)();

	bool open(ecs::MapId map, TransitionCallback callback = nullptr);
	bool open(std::string_view map_path, TransitionCallback callback = nullptr);
	bool close(TransitionCallback callback = nullptr);
	bool reset(TransitionCallback callback = nullptr);

	// The transition progress is a value between -1.f and 1.f. It is 0.f when not transitioning,
	// positive when transitioning out of a map, and negative when transitioning in to a map.
	float get_transition_progress();

	extern bool debug;

	void update(float dt);
}
