#include "stdafx.h"
#include "map.h"
#include "console.h"
#include "audio.h"
#include "ui_textboxes.h"
#include "ecs.h"
#include "ecs_tiled.h"

namespace map {
	ecs::MapId _current_map{};

	bool has_current_map() {
		return (bool)_current_map;
	}

	ecs::MapId get_current_map() {
		return _current_map;
	}

	// Returns nullptr if no music event is associated with the map.
	// TODO: this really belongs somewhere else!!!!!!!!!
	std::string_view _get_music_event_path_for_map(std::string_view map_path) {
		if (map_path.find("summer_forest") != std::string::npos)   return "event:/music/map/summer_forest";
		if (map_path.find("eternal_dungeon") != std::string::npos) return "event:/music/map/eternal_dungeon";
		return "";
	}

	ecs::MapId _next_map{};
	float _transition_duration = -1.f; // negative when not transitioning; zero when transitioning instantly; otherwise positive
	float _transition_progress = 1.f; // -1 to 1
	TransitionCallback _transition_callback = nullptr;

	enum class TransitionType {
		Open,
		Close,
		Reset,
	};

	struct TransitionOptions {
		TransitionType type = TransitionType::Open;
		ecs::MapId map{}; // Only used when type is Open.
		TransitionCallback callback = nullptr;
	};

	bool _transition(const TransitionOptions& options) {
		if (_transition_duration >= 0.f)
			return false; // already transitioning
		switch (options.type) {
		case TransitionType::Open: {
			if (options.map == _current_map)
				return false;
			_next_map = options.map;
		} break;
		case TransitionType::Close: {
			if (!_current_map)
				return false;
		} break;
		case TransitionType::Reset: {
			if (!_current_map)
				return false;
			_next_map = _current_map;
		} break;
		default:
			return false; // Should never happen.
		}
		constexpr float TRANSITION_DURATION = 0.7f; // seconds
		_transition_duration = TRANSITION_DURATION;
		_transition_callback = options.callback;
		return true;
	}

	bool open(ecs::MapId map, TransitionCallback callback) {
		if (!map)
			return false;
		TransitionOptions options{};
		options.type = TransitionType::Open;
		options.map = map;
		options.callback = callback;
		return _transition(options);
	}

	bool open(std::string_view map_path, TransitionCallback callback) {
		const ecs::MapId map = ecs::get_map(map_path);
		if (!map) {
			console::log_error("Failed to open map: " + std::string(map_path));
			return false;
		}
		open(map, callback);
	}

	bool close(TransitionCallback callback) {
		TransitionOptions options{};
		options.type = TransitionType::Close;
		options.callback = callback;
		return _transition(options);
	}

	bool reset(TransitionCallback callback) {
		TransitionOptions options{};
		options.type = TransitionType::Reset;
		options.callback = callback;
		return _transition(options);
	}

	float get_transition_progress() {
		return (_transition_duration >= 0.f) ? _transition_progress : 0.f;
	}

	void _show_debug_window(float dt) {
		ImGui::Begin("Maps");
		const std::string current_path(_current_map ? ecs::get_path(_current_map) : "");
		if (ImGui::BeginCombo("Map", current_path.c_str())) {
			for (ecs::MapId map : ecs::get_all_maps()) {
				const bool selected = (map == _current_map);
				const std::string path(ecs::get_path(map));
				if (ImGui::Selectable(path.c_str(), selected)) {
					open(map);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		if (ImGui::Button("Close")) close(); ImGui::SameLine();
		if (ImGui::Button("Reset")) reset();
		ImGui::Value("Transition Progress", _transition_progress);
		ImGui::End();
	}

	bool debug = false;

	void update(float dt) {
		if (debug) {
			_show_debug_window(dt);
		}

		if (_transition_duration < 0.f)
			return; // not transitioning

		const float delta_progress = _transition_duration ? (dt / _transition_duration) : 1.f;
		bool should_change_map = false;

		if (_transition_progress >= 0.f) {
			// transitioning out (progress goes from 0 to 1)
			_transition_progress += delta_progress;
			if (_transition_progress >= 1.f) {
				// finished transitioning out
				_transition_progress = -1.f;
				should_change_map = true;
			}
		} else {
			// transitioning in (progress goes from -1 to 0)
			_transition_progress += delta_progress;
			if (_transition_progress >= 0.f) {
				// finished transitioning in
				_transition_progress = 0.f;
				_transition_duration = -1.f; // stop transitioning
			}
		}

		if (!should_change_map)
			return;

		if (_current_map) {
			audio::stop_all_in_bus(audio::BUS_SOUND);
			ui::textboxes::close_all();
		}

		ecs::clear();

		if (_transition_callback) {
			_transition_callback();
			_transition_callback = nullptr;
		}

		_current_map = _next_map;
		_next_map = {};

		if (!_current_map) {
			audio::stop_all_in_bus();
			return;
		}

		ecs::setup(_current_map);

		// Make sure all systems have a chance to run right after the setup (and patching) is done.
		// This ensures all graphics are ready for rendering.
		ecs::update(0.f);

		const std::string music_event_path(_get_music_event_path_for_map(ecs::get_path(_current_map)));
		if (!music_event_path.empty()) {
			if (!audio::is_any_playing(music_event_path)) {
				audio::stop_all_in_bus(audio::BUS_MUSIC);
				audio::create_event(music_event_path);
			}
		}
	}
}
