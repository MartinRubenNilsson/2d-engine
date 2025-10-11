#include "stdafx.h"

namespace console {
	void _register_misc_commands();
	void _register_steam_commands();
	void _register_ecs_commands();

	void _register_commands() {
		_register_misc_commands();
		_register_steam_commands();
		_register_ecs_commands();
	}
}