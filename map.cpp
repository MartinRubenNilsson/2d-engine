#include "stdafx.h"
#include "map.h"
#include "map_grid.h"
#include "filesystem.h"
#include "console.h"
#include "audio.h"
#include "ui_textbox.h"
#include "ecs.h"
#include "ecs_tiled.h"

namespace map {
	const float DEFAULT_TRANSITION_DURATION = 0.6f; // seconds

	bool debug = false;
	std::string _current_map_path;
	std::string _next_map_path;
	float _transition_duration = -1.f; // negative when not transitioning; zero when transitioning instantly; otherwise positive
	float _transition_progress = 1.f; // -1 to 1

	void _show_debug_window(float dt) {
#ifdef _DEBUG_IMGUI
		ImGui::Begin("Maps");
		if (ImGui::BeginCombo("Map", _current_map_path.c_str())) {
			for (ecs::MapId map : ecs::get_all_maps()) {
				std::string_view path = ecs::get_path(map);
				bool is_selected = (_current_map_path == path);
				const std::string stem = filesystem::get_stem(path);
				if (ImGui::Selectable(stem.c_str(), is_selected)) {
					open(stem);
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		if (ImGui::Button("Close")) close(); ImGui::SameLine();
		if (ImGui::Button("Reset")) reset();
		ImGui::Value("Transition Progress", _transition_progress);
		ImGui::End();
#endif
	}

	// Returns nullptr if no music event is associated with the map.
	std::string_view _get_music_event_path_for_map(std::string_view map_path) {
		if (map_path.find("summer_forest") != std::string::npos)   return "event:/music/map/summer_forest";
		if (map_path.find("eternal_dungeon") != std::string::npos) return "event:/music/map/eternal_dungeon";
		return "";
	}

	void destroy_entities() {
		ecs::clear();
	}

	void update(float dt) {
		if (debug) {
			_show_debug_window(dt);
		}

		if (_transition_duration < 0.f) return; // not transitioning

		const float delta_progress = _transition_duration ? (dt / _transition_duration) : 1.f;
		bool shall_change_map = false;

		if (_transition_progress < 0.f) {
			// transitioning in (progress goes from -1 to 0)
			_transition_progress += delta_progress;
			if (_transition_progress >= 0.f) {
				// finished transitioning in
				_transition_progress = 0.f;
				_transition_duration = -1.f; // stop transitioning
			}
		} else {
			// transitioning out (progress goes from 0 to 1)
			_transition_progress += delta_progress;
			if (_transition_progress >= 1.f) {
				// finished transitioning out
				if (_next_map_path.empty()) {
					_transition_progress = 1.f;
					_transition_duration = -1.f; // stop transitioning
				} else {
					_transition_progress = -1.f;
				}
				shall_change_map = true;
			}
		}

		if (!shall_change_map) return;

		ecs::MapId current_map = ecs::get_map(_current_map_path);
		ecs::MapId next_map = ecs::get_map(_next_map_path);
		_current_map_path = _next_map_path;
		_next_map_path.clear();

		// CLOSE CURRENT MAP

		if (current_map) {
			audio::stop_all_in_bus(audio::BUS_SOUND);
			ui::close_textbox_and_clear_queue();
		}

		// OPEN NEXT MAP

		if (!next_map) {
			destroy_entities();
			destroy_grid();
			audio::stop_all_in_bus();
			return;
		}

		create_grid(next_map);
		ecs::setup(next_map);

		const std::string music_event_path(_get_music_event_path_for_map(_current_map_path));
		if (!music_event_path.empty()) {
			if (!audio::is_any_playing(music_event_path)) {
				audio::stop_all_in_bus(audio::BUS_MUSIC);
				audio::create_event({ .path = music_event_path });
			}
		}
	}

	bool transition(const MapTransitionOptions& options) {
		if (_transition_duration >= 0.f) return false; // already transitioning
		switch (options.type) {
		case MapTransitionType::Open: {
			if (options.map_name.empty()) return false;
			if (_current_map_path == options.map_name) return false;
			if (ecs::MapId map = ecs::get_map(options.map_name)) {
				_next_map_path = ecs::get_path(map);
			} else {
				console::log_error("Map not found: " + std::string(options.map_name));
				return false;
			}
		} break;
		case MapTransitionType::Close: {
			if (_current_map_path.empty()) return false;
		} break;
		case MapTransitionType::Reset: {
			if (_current_map_path.empty()) return false;
			_next_map_path = _current_map_path;
		} break;
		default:
			return false;
		}
		_transition_duration = std::max(options.duration, 0.f);
		return true;
	}

	bool is_open() {
		return !_current_map_path.empty();
	}

	bool open(std::string_view map_name, float transition_duration) {
		MapTransitionOptions options{};
		options.type = MapTransitionType::Open;
		options.map_name = map_name;
		options.duration = transition_duration;
		return transition(options);
	}

	bool close(float transition_duration) {
		MapTransitionOptions options{};
		options.type = MapTransitionType::Close;
		options.duration = transition_duration;
		return transition(options);
	}

	bool reset(float transition_duration) {
		MapTransitionOptions options{};
		options.type = MapTransitionType::Reset;
		options.duration = transition_duration;
		return transition(options);
	}

	std::string get_name() {
		return filesystem::get_stem(_current_map_path);
	}

	float get_transition_progress() {
		return (_transition_duration >= 0.f) ? _transition_progress : 0.f;
	}

	bool is_dark() {
		return get_name().starts_with("muddy_cave"); // HACK
	}
}
