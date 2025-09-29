#include "stdafx.h"
#include "map.h"
#include "map_grid.h"
#include "map_entities.h"
#include "tiled.h"
#include "tiled_types.h"
#include "filesystem.h"
#include "console.h"
#include "audio.h"
#include "ui_textbox.h"

namespace map {
	const float DEFAULT_TRANSITION_DURATION = 0.6f; // seconds

	bool debug = false;
	tiled::Context _tiled_context;
	std::string _current_map_path;
	std::string _next_map_path;
	unsigned int _object_layer_index = 0;
	unsigned int _next_free_layer_index = 0;
	float _transition_duration = -1.f; // negative when not transitioning; zero when transitioning instantly; otherwise positive
	float _transition_progress = 1.f; // -1 to 1
	std::unordered_map<std::string, MapPatch> _map_path_to_patch;

	void _show_debug_window(float dt) {
#ifdef _DEBUG_IMGUI
		ImGui::Begin("Maps");
		if (ImGui::BeginCombo("Map", _current_map_path.c_str())) {
			for (const tiled::Map& map : _tiled_context.maps) {
				bool is_selected = (_current_map_path == map.path);
				const std::string stem = filesystem::get_stem(map.path);
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

	bool _tiled_file_load_callback(std::string_view path, std::string& contents) {
		return filesystem::read_text_file(path, contents);
	}

	void _tiled_debug_message_callback(std::string_view message) {
		__debugbreak();
		console::log_error(message);
	}

	void initialize() {
		_tiled_context.file_load_callback = _tiled_file_load_callback;
		_tiled_context.debug_message_callback = _tiled_debug_message_callback;

		// Preload all Tiled assets.
		const std::span<const filesystem::File> files = filesystem::get_all_files_in_directory("assets/tiled");
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledTileset) continue;
			tiled::load_tileset_from_file(_tiled_context, file.path);
		}
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledTemplate) continue;
			tiled::load_template_from_file(_tiled_context, file.path);
		}
		for (const filesystem::File& file : files) {
			if (file.format != filesystem::FileFormat::TiledMap) continue;
			tiled::load_map_from_file(_tiled_context, file.path);
		}
	}

	const tiled::Map* _find_map_by_path(std::string_view path) {
		if (path.empty()) return nullptr;
		for (const tiled::Map& map : _tiled_context.maps) {
			if (map.path == path) {
				return &map;
			}
		}
		return nullptr;
	}

	const tiled::Map* _find_map_by_path_stem(std::string_view stem) {
		if (stem.empty()) return nullptr;
		for (const tiled::Map& map : _tiled_context.maps) {
			if (filesystem::get_stem(map.path) == stem) {
				return &map;
			}
		}
		return nullptr;
	}

	// Returns nullptr if no music event is associated with the map.
	std::string_view _get_music_event_path_for_map(std::string_view map_path) {
		if (map_path.find("summer_forest") != std::string::npos)   return "event:/music/map/summer_forest";
		if (map_path.find("eternal_dungeon") != std::string::npos) return "event:/music/map/eternal_dungeon";
		return "";
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

		const tiled::Map* current_map = _find_map_by_path(_current_map_path);
		const tiled::Map* next_map = _find_map_by_path(_next_map_path);
		_current_map_path = _next_map_path;
		_next_map_path.clear();

		// CLOSE CURRENT MAP

		if (current_map) {
			_object_layer_index = 0;
			_next_free_layer_index = 0;
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

		// Resolve the layer index on which to place objects/entities.
		for (unsigned int layer_index = 0; layer_index < next_map->layers.size(); ++layer_index) {
			const tiled::Layer& layer = next_map->layers[layer_index];
			if (layer.type == tiled::LayerType::Tile) {
				if (layer.name.starts_with("object") || layer.name.starts_with("Object")) {
					_object_layer_index = layer_index;
					break;
				}
			} else if (layer.type == tiled::LayerType::Object) {
				_object_layer_index = layer_index;
				break;
			}
		}
		_next_free_layer_index = (unsigned int)next_map->layers.size();

		create_grid(*next_map);
		create_entities(*next_map);
		patch_entities(_map_path_to_patch[_current_map_path]);

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
			if (const tiled::Map* map = _find_map_by_path_stem(options.map_name)) {
				_next_map_path = map->path;
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

	unsigned int get_object_layer_index() {
		return _object_layer_index;
	}

	unsigned int get_next_free_layer_index() {
		return _next_free_layer_index;
	}

	unsigned int find_tileset_by_name(std::string_view name) {
		if (name.empty()) return UINT_MAX;
		for (unsigned int tileset_id = 0; tileset_id < _tiled_context.tilesets.size(); ++tileset_id) {
			if (_tiled_context.tilesets[tileset_id].name == name) {
				return tileset_id;
			}
		}
		return UINT_MAX;
	}

	const tiled::Tileset* get_tileset(unsigned int tileset_id) {
		if (tileset_id >= _tiled_context.tilesets.size()) return nullptr;
		return &_tiled_context.tilesets[tileset_id];
	}

	float get_transition_progress() {
		return (_transition_duration >= 0.f) ? _transition_progress : 0.f;
	}

	bool is_dark() {
		return get_name().starts_with("muddy_cave"); // HACK
	}

	MapPatch* _get_patch() {
		if (_current_map_path.empty()) return nullptr;
		return &_map_path_to_patch[_current_map_path];
	}

	template <typename T>
	bool _insert_into_sorted_vector(std::vector<T>& vec, const T& value) {
		auto it = std::lower_bound(vec.begin(), vec.end(), value);
		if (it != vec.end() && *it == value) return false;
		vec.insert(it, value);
		return true;
	}

	void mark_entity_as_destroyed(entt::entity entity) {
		MapPatch* patch = _get_patch();
		if (!patch) return;
		_insert_into_sorted_vector(patch->destroyed_entities, entity);
	}

	void mark_chest_as_opened(entt::entity entity) {
		MapPatch* patch = _get_patch();
		if (!patch) return;
		_insert_into_sorted_vector(patch->opened_chests, entity);
	}
}
