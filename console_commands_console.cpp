#include "stdafx.h"
#include "console_commands.h"
#include "console.h"

namespace console {
	void _execute_help_command(Args args) {
		if (const Command* command = find_command_with_name(get_string(args[0]))) {
			log(get_command_help_message(*command));
		}
	}

#if 0
	void _execute_toggle_command(Args args) {
		const std::string command_name = get_string(args[0]);
		const Command* command = find_command_with_name(command_name);
		if (!command) {
			log_error("\"" + command_name + "\" is not a command");
			return;
		}
		if (command->params.size() == 0) {
			log_error(command_name + " does not take any parameters");
			return;
		}
		if (command->params.size() > 1) {
			log_error(command_name + " takes more than one parameter");
			return;
		}
		if (command->params[0].type != ParamType::Bool) {
			log_error(command_name + " does not take a bool parameter");
			return;
		}
		static std::unordered_map<std::string, bool> value; //HACK
	}
#endif

	void _create_commands_console() {
		create_command({
			.name = "help",
			.desc = "Shows help for a command",
			.params = { { ParamType::String, "command", "The command to show help for" } },
			.execute = _execute_help_command,
			});
		create_command({
			.name = "clear",
			.desc = "Clears the console",
			.execute = [](Args args) { clear(); }
			});
		create_command({
			.name = "sleep",
			.desc = "Defers incoming commands, executing them later",
			.params = { { ParamType::Float, "seconds", "The number of seconds to sleep" } },
			.execute = [](Args args) { sleep(get_float(args[0])); }
			});
		create_command({
			.name = "log",
			.desc = "Logs a message to the console",
			.params = { { ParamType::String, "message", "The message to log" } },
			.execute = [](Args args) { log(get_string(args[0])); }
			});
		create_command({
			.name = "log_error",
			.desc = "Logs an error message to the console",
			.params = { { ParamType::String, "message", "The message to log" } },
			.execute = [](Args args) { log_error(get_string(args[0])); }
			});
		create_command({
			.name = "execute",
			.desc = "Executes a console command",
			.params = { { ParamType::String, "command_line", "The command to execute" } },
			.execute = [](Args args) { execute(get_string(args[0])); }
			});
		create_command({
			.name = "execute_script",
			.desc = "Executes a console script",
			.params = { { ParamType::String, "script_name", "The name of the script" } },
			.execute = [](Args args) { execute_script_from_file(get_string(args[0])); }
			});
		create_command({
			.name = "bind",
			.desc = "Binds a console command to a key",
			.params = {
				{ ParamType::String, "key", "The key to bind" },
			{ ParamType::String, "command_line", "The command to execute" }
			},
			.execute = [](Args args) { console::bind(get_string(args[0]), get_string(args[1])); }
			});
		create_command({
			.name = "unbind",
			.desc = "Unbinds a key",
			.params = { { ParamType::String, "key", "The key to unbind" } },
			.execute = [](Args args) { unbind(get_string(args[0])); }
			});
#if 0
		register_command({
			.name = "toggle",
			.desc = "Toggles any command that takes a single BOOL parameter.",
			.params = { { ParamType::String, "command", "The command to toggle" } },
			.callback = _execute_toggle_command
			});
#endif
	}
}