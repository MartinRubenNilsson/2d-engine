#pragma once

namespace platform {
	int system(const std::string& command);
	bool set_environment_variable(const std::string& name, const std::string& value);
	bool open(const std::string& path);

	void shutdown_directory_watchers();
}