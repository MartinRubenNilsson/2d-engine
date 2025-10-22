#include "stdafx.h"
#include "console.h"
#include "console_commands.h"
#include "window_events.h"
#include "files.h"
#include <deque> // TODO: use a ringbuffer instead
#include <iostream>

namespace console {
	const size_t _MAX_HISTORY = 512;

	bool _visible = false;
	bool _has_focus = false;
	bool _reclaim_focus = false;
	float _sleep_timer = 0.f;
	std::deque<std::string> _command_queue; // TODO: use a ringbuffer instead
	std::deque<std::string> _command_history; // TODO: use a ringbuffer instead
	std::deque<std::string>::iterator _command_history_it = _command_history.end(); // TODO: use a ringbuffer instead

	struct HistoryEntry {
		std::string message;
		Color color = Color::WHITE;
		unsigned int repeat = 0;
	};

	std::deque<HistoryEntry> _history; // TODO: use a ringbuffer instead

	std::stringstream _cout_stream;
	std::stringstream _cerr_stream;

	void startup() {
		// Redirect cout and cerr to stringstreams so we can capture
		// anything that gets written to them.
		std::cout.rdbuf(_cout_stream.rdbuf());
		std::cerr.rdbuf(_cerr_stream.rdbuf());
		create_commands();
	}

	int _input_text_callback(ImGuiInputTextCallbackData* data) {

		// COMPLETE COMMANDS

		if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
			const std::string_view prefix(data->Buf, data->Buf + data->BufTextLen);
			const std::span<const Command> commands = find_commands_whose_name_starts_with(prefix);
			if (commands.empty()) return 0;
			if (commands.size() == 1) {
				const std::string_view command_name = commands.front().name;
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, command_name.data(), command_name.data() + command_name.size());
				return 0;
			}
			for (const Command& command : commands) {
				log(command.name);
			}
			return 0;
		}

		// NAVIGATE COMMAND HISTORY

		if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
			if (_command_history.empty()) return 0;
			if (data->EventKey == ImGuiKey_UpArrow) {
				if (_command_history_it > _command_history.begin()) {
					_command_history_it--;
					data->DeleteChars(0, data->BufTextLen);
					data->InsertChars(0, _command_history_it->c_str());
				}
			} else if (data->EventKey == ImGuiKey_DownArrow) {
				if (_command_history_it < _command_history.end() - 1) {
					_command_history_it++;
					data->DeleteChars(0, data->BufTextLen);
					data->InsertChars(0, _command_history_it->c_str());
				}
			}
		}

		return 0;
	}

	void update(float dt) {
		// LOG COUT AND CERR
		{
			std::string line;
			while (std::getline(_cout_stream, line)) {
				log(line);
			}
			_cout_stream.str(std::string());
			while (std::getline(_cerr_stream, line)) {
				log_error(line);
			}
			_cerr_stream.str(std::string());
		}

		// UPDATE SLEEP TIMER

		if (_sleep_timer > 0.f) {
			_sleep_timer -= dt;
			if (_sleep_timer < 0.f)
				_sleep_timer = 0.f;
		}

		// EXECUTE COMMAND QUEUE

		while (!_sleep_timer && !_command_queue.empty()) {
			execute(_command_queue.front());
			_command_queue.pop_front();
		}

		// SHOW CONSOLE WINDOW

		if (!_visible) return;

		ImVec2 size = ImGui::GetIO().DisplaySize;
		size.x *= 0.75f;
		size.y *= 0.5f;
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(size);
		ImGui::SetNextWindowBgAlpha(0.75f); // Transparent background

		ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings;

		if (ImGui::Begin("Console", &_visible, window_flags)) {

			// HISTORY

			const float reserved_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
			if (ImGui::BeginChild("History", ImVec2(0, -reserved_height), // Leave room for 1 separator + 1 input text
				false, ImGuiWindowFlags_HorizontalScrollbar))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
				for (const auto& [line, color, repeat] : _history) {
					const ImColor col(color.r, color.g, color.b, color.a);
					if (repeat == 0) {
						ImGui::TextColored(col, line.c_str());
					} else {
						ImGui::TextColored(col, "%s (%u)", line.c_str(), repeat + 1);
					}
				}
				ImGui::PopStyleVar();
				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
					ImGui::SetScrollHereY(1.0f); // Scroll to bottom
			}
			ImGui::EndChild();

			ImGui::Separator();

			// COMMAND LINE

			static std::string command_line;
			ImGui::PushItemWidth(-1); // Use all available width
			if (ImGui::InputText("##CommandLine", &command_line,
				ImGuiInputTextFlags_EnterReturnsTrue |
				ImGuiInputTextFlags_CallbackCompletion |
				ImGuiInputTextFlags_CallbackHistory |
				ImGuiInputTextFlags_EscapeClearsAll,
				_input_text_callback))
			{
				execute(command_line);
				command_line.clear();
				_reclaim_focus = true;
			}
			ImGui::PopItemWidth();

			// FOCUS

			ImGui::SetItemDefaultFocus();
			if (_reclaim_focus) {
				ImGui::SetKeyboardFocusHere(-1); // Auto focus command line
				_reclaim_focus = false;
			}
			_has_focus = ImGui::IsWindowFocused();
		}
		ImGui::End();
	}

	void handle_window_events() {
		if (ImGui::GetIO().WantCaptureKeyboard)
			return; // Don't execute bound commands while we're typing in console.

		for (const window::Event& ev : window::get_events()) {
			if (ev.type != window::EventType::KeyPress)
				continue;
			if (ev.key.code == window::Key::GraveAccent) {
				console::toggle_visible();
			} else {
				execute_bind(ev.key.code);
			}
		}
	}

	bool is_visible() {
		return _visible;
	}

	void set_visible(bool visible) {
		_visible = visible;
		if (_visible) {
			_reclaim_focus = true;
		} else {
			_has_focus = false;
		}
	}

	void toggle_visible() {
		_visible = !_visible;
		if (_visible) {
			_reclaim_focus = true;
		} else {
			_has_focus = false;
		}
	}

	bool has_focus() {
		return _has_focus;
	}

	void clear() {
		_history.clear();
	}

	void sleep(float seconds) {
		_sleep_timer = std::max(0.f, seconds);
	}

	void _add_to_history(std::string_view message, Color color) {
		if (_history.empty()) {
			_history.emplace_back(std::string(message), color);
			return;
		}
		if (!_history.empty() && _history.back().message == message) {
			_history.back().repeat++;
			return;
		}
		_history.emplace_back(std::string(message), color);
		if (_history.size() > _MAX_HISTORY) {
			_history.pop_front();
		}
	}

	void log(std::string_view message) {
		constexpr Color color(252, 191, 73, 255);
		_add_to_history(message, color);
	}

	void log_error(std::string_view message, bool show_console) {
		constexpr Color color(220, 50, 47, 255);
		_add_to_history(message, color);
		if (show_console) {
			_visible = true;
		}
	}

	void execute(std::string_view command_line, bool defer) {
		if (command_line.starts_with("//")) return; // ignore comments
		if (defer) {
			_command_queue.emplace_back(command_line);
			return;
		}
		_command_history.emplace_back(command_line);
		if (_command_history.size() > _MAX_HISTORY) {
			_command_history.pop_front();
		}
		_command_history_it = _command_history.end();
		const Color color(230, 230, 230, 255);
		_add_to_history(command_line, color);
		parse_and_execute_command(command_line);
	}

	void execute(int argc, char* argv[]) {
		if (argc == 1) return;
		std::string command_line;
		for (int i = 1; i < argc; ++i) {
			command_line += argv[i];
			if (i < argc - 1) {
				command_line += ' ';
			}
		}
		execute(command_line);
	}

	void execute_script_from_file(std::string_view path) {
		std::string script;
		if (!files::read_text_file(path, script)) {
			log_error("Failed to open console script: " + std::string(path));
			return;
		}
		std::istringstream script_stream(std::move(script));
		for (std::string line; std::getline(script_stream, line);) {
			execute(line, true);
		}
	}

	std::unordered_map<window::Key, std::string> _key_bindings; // Maps keys to command lines

	void bind(window::Key key, std::string_view command_line) {
		_key_bindings[key] = command_line;
	}

	void bind(std::string_view key_string, std::string_view command_line) {
		auto key = magic_enum::enum_cast<window::Key>(key_string, magic_enum::case_insensitive);
		const auto array = magic_enum::enum_entries<window::Key>();
		if (key.has_value()) {
			bind(key.value(), command_line);
		} else {
			log_error("Failed to bind key: " + std::string(key_string));
		}
	}

	void unbind(window::Key key) {
		_key_bindings.erase(key);
	}

	void unbind(std::string_view key_string) {
		auto key = magic_enum::enum_cast<window::Key>(key_string, magic_enum::case_insensitive);
		if (key.has_value()) {
			unbind(key.value());
		} else {
			log_error("Failed to unbind key: " + std::string(key_string));
		}
	}

	void execute_bind(window::Key key) {
		auto it = _key_bindings.find(key);
		if (it == _key_bindings.end()) return;
		execute(it->second);
	}
}
