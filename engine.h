#pragma once

namespace engine {
	void startup(int argc, char* argv[]);
	void shutdown();
	bool should_run();
	void run();
}