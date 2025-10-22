#include "stdafx.h"

namespace console {
	void _create_misc_commands();
	void _create_steam_commands();
	void _create_ecs_commands();

	void _create_commands() {
		_create_misc_commands();
		_create_steam_commands();
		_create_ecs_commands();
	}
}