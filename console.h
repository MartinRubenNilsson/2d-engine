#pragma once

namespace window {
	enum class Key;
}

namespace console {
	void startup();
	void update(float dt);
	void handle_window_events();

	bool is_visible();
	void set_visible(bool visible);
	void toggle_visible();
	bool has_focus();
	void clear();
	void sleep(float seconds);
	void log(std::string_view message);
	void log_error(std::string_view message, bool show_console = true);
	void execute(std::string_view command_line, bool defer = false);
	void execute(int argc, char* argv[]);
	void execute_script_from_file(std::string_view path);
	void bind(window::Key key, std::string_view command_line);
	void bind(std::string_view key_string, std::string_view command_line);
	void unbind(window::Key key);
	void unbind(std::string_view key_string);
	void execute_bind(window::Key key);
}
