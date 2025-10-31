#include "stdafx.h"
#include "console_commands.h"
#include "engine_fps_counter.h"
#include "audio.h"
#include "map.h"
#include "ecs_lifetime.h"
#include "editor.h"

namespace console {
	void _create_commands_misc() {

		// ENGINE

		create_command({
			.name = "toggle_fps_counter",
			.desc = "Toggles whether the FPS counter is visible or hidden.",
			.execute = [](Args) { engine::should_show_fps_counter = !engine::should_show_fps_counter; }
			});

		// EDITOR

		create_command({
			.name = "toggle_editor",
			.desc = "Toggles whether the editor should be visible.",
			.execute = [](Args) { editor::show = !editor::show; }
			});

		// SHADERS

#if 0
		add_command({
			.name = "reload_shaders",
			.desc = "Reloads all shaders",
			.callback = [](Args args) {
				shaders::reload_assets();
			}
			});
#endif

		// AUDIO

		create_command({
			.name = "audio_play",
			.desc = "Plays an audio event",
			.params = { { ParamType::String, "event_path", "The full path of the event" } },
			.execute = [](Args args) {
				audio::create_event(get_string(args[0]));
			}
			});

		// MAP

		create_command({
			.name = "open_map",
			.desc = "Opens a map",
			.params = { { ParamType::String, "name", "The name of the map" } },
			.execute = [](Args args) { map::open(get_string(args[0])); }
			});
		create_command({
			.name = "close_map",
			.desc = "Closes the current map",
			.execute = [](Args) { map::close(); }
			});
		create_command({
			.name = "reset_map",
			.desc = "Resets the current map",
			.execute = [](Args) { map::reset(); }
			});

		// GAME

		create_command({
			.name = "destroy",
			.desc = "Destroy an entity",
			.params = {
				{ ParamType::Int, "entity", "The ID of the entity to destroy" },
			},
			.execute = [](Args args) {
				ecs::destroy_later((entt::entity)get_int(args[0]));
			}
			});
#if 0
		add_command({
			.name = "clone",
			.desc = "Clone an entity",
			.params = {
				{ ParamType::Int, "entity", "The ID of the entity to clone" },
			},
			.callback = [](Args args) {
				ecs::deep_copy((entt::entity)get_int(args[0]));
			}
			});
		register_command({
			.name = "kill_player",
			.desc = "Kills the player",
			.callback = [](Args args) {
				ecs::kill_player(ecs::find_entity_with_tag(ecs::Tag::Player));
			}
			});
		register_command({
			.name = "add_camera_shake",
			.desc = "Adds trauma to the active camera to make it shake",
			.params = {
				{ ParamType::Float, "trauma", "The amount of trauma to add" },
			},
			.callback = [](Args args) {
				ecs::add_trauma_to_active_camera(get_float(args[0]));
			}
			});
		register_command({
			.name = "create_vfx",
			.desc = "Spawns a VFX in the game world",
			.params = {
				{ ParamType::String, "type", "The type of VFX" },
				{ ParamType::Vec2f, "position", "The position to spawn the VFX at" },
			},
			.callback = [](Args args) {
				std::string type_str = get_string(args[0]);
				Vec2f position = get_vec2f(args[1]);
				auto type = magic_enum::enum_cast<ecs::VfxType>(type_str, magic_enum::case_insensitive);
				if (type.has_value()) {
					create_vfx(type.value(), position);
				} else {
					log_error("Unknown VFX type: " + type_str);
				}
			}
			});
#endif
	}
}