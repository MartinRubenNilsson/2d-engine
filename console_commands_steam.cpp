#include "stdafx.h"
#include "console_commands.h"
#include "console.h"
#include "steam.h"
#include "steam_server.h"
#include "steam_client.h"

namespace console {
	void _create_steam_commands() {
		create_command({
			.name = "steam_id",
			.desc = "Prints the Steam ID of the current user",
			.callback = [](Args) {
				log(steam::get_steam_id());
			}
		});
		create_command({
			.name = "steam_friends",
			.desc = "Prints the names of the friends of the current user",
			.callback = [](Args) {
				auto names = steam::get_friends_persona_names();
				for (const std::string& name : names)
					log(name);
			}
		});
		create_command({
			.name = "server_init",
			.desc = "Initializes the server",
			.callback = [](Args) {
				steam::server_initialize();
			}
		});
		create_command({
			.name = "server_shutdown",
			.desc = "Shuts down the server",
			.callback = [](Args) {
				steam::server_shutdown();
			}
		});
		create_command({
			.name = "server_ip",
			.desc = "Prints the public IP of the server",
			.callback = [](Args) {
				log(steam::server_get_public_ip());
			}
		});
#if 0
		add_command({
			.name = "server_log_on_anonymous",
			.desc = "Logs on the server anonymously",
			.callback = [](Args) {
				steam::server_log_on_anonymous();
			}
		});
		add_command({
			.name = "server_log_off",
			.desc = "Logs off the server",
			.callback = [](Args) {
				steam::server_log_off();
			}
		});
		add_command({
			.name = "server_logged_on",
			.desc = "Prints whether the server is logged on",
			.callback = [](Args) {
				log(steam::server_logged_on() ? "true" : "false");
			}
		});
#endif
		create_command({
			.name = "client_connect",
			.desc = "Connects the client to a server",
			.params = {
				{ ParamType::String, "IP", "The IP address of the server" },
			},
			.callback = [](Args args) {
				steam::connect_client_to_server_by_ip(get_string(args[0]));
			}
		});
	}
}