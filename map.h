#pragma once
#include <string_view>

namespace map {
	extern const float DEFAULT_TRANSITION_DURATION; // seconds
	extern bool debug;

	enum class MapTransitionType {
		Open, // Open a new map.
		Close, // Close the current map.
		Reset, // Reset the current map.
	};

	struct MapTransitionOptions {
		MapTransitionType type = MapTransitionType::Open;
		std::string_view map_name; // Only used when type is Open.
		float duration = DEFAULT_TRANSITION_DURATION; // In seconds; set to 0 to make the transition instant.
	};

	void update(float dt);

	bool transition(const MapTransitionOptions& options);
	bool is_open();
	bool open(std::string_view map_name, float transition_duration = DEFAULT_TRANSITION_DURATION);
	bool close(float transition_duration = DEFAULT_TRANSITION_DURATION);
	bool reset(float transition_duration = DEFAULT_TRANSITION_DURATION);

	std::string get_name();
	unsigned int get_object_layer_index();
	unsigned int get_next_free_layer_index(); // one past the last layer index

	// The transition progress is a value between -1 and 1. It is 0 when not transitioning,
	// positive when transitioning out of a map, and negative when transitioning in to a map.
	float get_transition_progress();
	bool is_dark();
}
