#include "stdafx.h"
#include "map.h"
#include "files.h"
#include "console.h"
#include "audio.h"
#include "ui_textboxes.h"
#include "ecs.h"
#include "ecs_tiled.h"

namespace map {
	bool debug = false;

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
	void (*on_transition_complete)() = nullptr;

	void _show_debug_window(float dt) {
		ImGui::Begin("Maps");
		const std::string current_path(_current_map ? ecs::get_path(_current_map) : "");
		if (ImGui::BeginCombo("Map", current_path.c_str())) {
			for (ecs::MapId map : ecs::get_all_maps()) {
				const bool selected = (map == _current_map);
				const std::string path(ecs::get_path(map));
				if (ImGui::Selectable(path.c_str(), selected)) {
					open(path);
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

		if (on_transition_complete) {
			on_transition_complete();
			on_transition_complete = nullptr;
		}

		_current_map = _next_map;
		_next_map = {};
		
		if (!_current_map) {
			audio::stop_all_in_bus();
			return;
		}

		ecs::setup(_current_map);
		ecs::update(0.f); // HACK: make sure cameras and such are initialized

		const std::string music_event_path(_get_music_event_path_for_map(ecs::get_path(_current_map)));
		if (!music_event_path.empty()) {
			if (!audio::is_any_playing(music_event_path)) {
				audio::stop_all_in_bus(audio::BUS_MUSIC);
				audio::create_event(music_event_path);
			}
		}
	}

	enum class TransitionType {
		Open,
		Close,
		Reset,
	};

	const float DEFAULT_TRANSITION_DURATION = 0.7f; // seconds

	struct TransitionOptions {
		TransitionType type = TransitionType::Open;
		std::string_view path; // Only used when type is Open.
		float duration = DEFAULT_TRANSITION_DURATION; // In seconds; set to 0 to make the transition instant.
		void(*on_complete)() = nullptr;
	};

	bool _transition(const TransitionOptions& options) {
		if (_transition_duration >= 0.f)
			return false; // already transitioning
		switch (options.type) {
		case TransitionType::Open: {
			const ecs::MapId map = ecs::get_map(options.path);
			if (!map) {
				console::log_error("Map not found: " + std::string(options.path));
				return false;
			}
			if (map == _current_map)
				return false;
			_next_map = map;
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
			return false;
		}
		_transition_duration = std::max(options.duration, 0.f);
		on_transition_complete = options.on_complete;
		return true;
	}

	bool open(std::string_view path, void(*on_complete)(), float duration) {
		TransitionOptions options{};
		options.type = TransitionType::Open;
		options.path = path;
		options.duration = duration;
		options.on_complete = on_complete;
		return _transition(options);
	}

	bool close(void(*on_complete)(), float duration) {
		TransitionOptions options{};
		options.type = TransitionType::Close;
		options.duration = duration;
		options.on_complete = on_complete;
		return _transition(options);
	}

	bool reset(void(*on_complete)(), float duration) {
		TransitionOptions options{};
		options.type = TransitionType::Reset;
		options.duration = duration;
		options.on_complete = on_complete;
		return _transition(options);
	}

	float get_transition_progress() {
		return (_transition_duration >= 0.f) ? _transition_progress : 0.f;
	}
}
