#pragma once
#include <string_view>

namespace map {
	extern bool debug;

	void update(float dt);
	
	bool is_open();

	extern const float DEFAULT_TRANSITION_DURATION; // seconds

	bool open(std::string_view path, void(*on_complete)() = nullptr, float duration = DEFAULT_TRANSITION_DURATION);
	bool close(void(*on_complete)() = nullptr, float duration = DEFAULT_TRANSITION_DURATION);
	bool reset(void(*on_complete)() = nullptr, float duration = DEFAULT_TRANSITION_DURATION);

	// The transition progress is a value between -1 and 1. It is 0 when not transitioning,
	// positive when transitioning out of a map, and negative when transitioning in to a map.
	float get_transition_progress();
	bool is_dark();
}
