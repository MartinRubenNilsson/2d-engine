#include "stdafx.h"
#include "console_commands.h"
#include "console.h"
//#include "window.h"
#include "audio.h"
#include "map.h"
#include "ui.h"
//#include "shaders.h"
#include "ecs.h"
#include "ecs_player.h"
#include "ecs_lifetime.h"
#include "ecs_camera.h"
#include "ecs_vfx.h"
#include "ecs_tags.h"

namespace console {
	void _add_misc_commands() {

		// CONSOLE

		add_command({
			.name = "help",
			.desc = "Shows help for a command",
			.params = {
				{ ParamType::String, "command", "The command to show help for" },
			},
			.callback = [](Args args) {
				if (const Command* command = find_command_with_name(get_string(args[0]))) {
					log(format_command_help_message(*command));
				}
			}
		});
		add_command({
			.name = "clear",
			.desc = "Clears the console",
			.callback = [](Args args) {
				clear();
			}
		});
		add_command({
			.name = "sleep",
			.desc = "Defers incoming commands, executing them later",
			.params = {
				{ ParamType::Float, "seconds", "The number of seconds to sleep" },
			},
			.callback = [](Args args) {
				sleep(get_float(args[0]));
			}
		});
		add_command({
			.name = "log",
			.desc = "Logs a message to the console",
			.params = {
				{ ParamType::String, "message", "The message to log" },
			},
			.callback = [](Args args) {
				log(get_string(args[0]));
			}
		});
		add_command({
			.name = "log_error",
			.desc = "Logs an error message to the console",
			.params = {
				{ ParamType::String, "message", "The message to log" },
			},
			.callback = [](Args args) {
				log_error(get_string(args[0]));
			}
		});
		add_command({
			.name = "execute",
			.desc = "Executes a console command",
			.params = {
				{ ParamType::String, "command_line", "The command to execute" },
			},
			.callback = [](Args args) {
				execute(get_string(args[0]));
			}
		});
		add_command({
			.name = "execute_script",
			.desc = "Executes a console script",
			.params = {
				{ ParamType::String, "script_name", "The name of the script" },
			},
			.callback = [](Args args) {
				execute_script_from_file(get_string(args[0]));
			}
		});
		add_command({
			.name = "bind",
			.desc = "Binds a console command to a key",
			.params = {
				{ ParamType::String, "key", "The key to bind" },
				{ ParamType::String, "command_line", "The command to execute" },
			},
			.callback = [](Args args) {
				console::bind(get_string(args[0]), get_string(args[1]));
			}
		});
		add_command({
			.name = "unbind",
			.desc = "Unbinds a key",
			.params = {
				{ ParamType::String, "key", "The key to unbind" },
			},
			.callback = [](Args args) {
				console::unbind(get_string(args[0]));
			}
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

		add_command({
			.name = "audio_play",
			.desc = "Plays an audio event",
			.params = {
				{ ParamType::String, "event_path", "The full path of the event" },
			},
			.callback = [](Args args) {
				audio::create_event({ .path = get_string(args[0]) });
			}
		});

		// MAP

		add_command({
			.name = "map_open",
			.desc = "Opens a map",
			.params = {
				{ ParamType::String, "name", "The name of the map" },
			},
			.callback = [](Args args) {
				map::open(get_string(args[0]));
			}
		});
		add_command({
			.name = "map_close",
			.desc = "Closes the current map",
			.callback = [](Args args) {
				map::close();
			}
		});
		add_command({
			.name = "map_reset",
			.desc = "Resets the current map",
			.callback = [](Args args) {
				map::reset();
			}
		});

		// UI

		add_command({
			.name = "ui_show",
			.desc = "Shows an RML document",
			.params = {
				{ ParamType::String, "name", "The name of the document" },
			},
			.callback = [](Args args) {
				ui::show_document(get_string(args[0]));
			}
		});

		// GAME

		add_command({
			.name = "destroy",
			.desc = "Destroy an entity",
			.params = {
				{ ParamType::Int, "entity", "The ID of the entity to destroy" },
			},
			.callback = [](Args args) {
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
#endif
		add_command({
			.name = "kill_player",
			.desc = "Kills the player",
			.callback = [](Args args) {
				ecs::kill_player(ecs::find_entity_with_tag(ecs::Tag::Player));
			}
		});
		add_command({
			.name = "add_camera_shake",
			.desc = "Adds trauma to the active camera to make it shake",
			.params = {
				{ ParamType::Float, "trauma", "The amount of trauma to add" },
			},
			.callback = [](Args args) {
				ecs::add_trauma_to_active_camera(get_float(args[0]));
			}
		});
		add_command({
			.name = "create_vfx",
			.desc = "Spawns a VFX in the game world",
			.params = {
				{ ParamType::String, "type", "The type of VFX" },
				{ ParamType::Vec2f, "position", "The position to spawn the VFX at" },
			},
			.callback = [](Args args) {
				std::string type_str = get_string(args[0]);
				Vec2f position = get_vector2f(args[1]);
				auto type = magic_enum::enum_cast<ecs::VfxType>(type_str, magic_enum::case_insensitive);
				if (type.has_value()) {
					create_vfx(type.value(), position);
				} else {
					log_error("Unknown VFX type: " + type_str);
				}
			}
		});
	}
}