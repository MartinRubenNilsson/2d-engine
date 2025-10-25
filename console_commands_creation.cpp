#include "stdafx.h"

namespace console {
	void _create_commands_console();
	void _create_commands_steam();
	void _create_commands_ui();
	void _create_commands_ecs();
	void _create_commands_misc();

	void _create_commands() {
		_create_commands_console();
		_create_commands_ui();
		_create_commands_steam();
		_create_commands_ecs();
		_create_commands_misc();
	}
}