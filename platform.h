#pragma once

namespace platform {
	int system(const char* command);
	bool set_environment_variable(const char* name, const char* value);
	bool open(const char* path);

	void shutdown();
}