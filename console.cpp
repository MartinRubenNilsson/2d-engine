#include "stdafx.h"
#include "console.h"
#include "console_commands.h"
#include "window_events.h"
#include "filesystem.h"
#include <deque> // TODO: use a ringbuffer instead

namespace console {
	const Color _COLOR_COMMAND = Color(230, 230, 230, 255);
	const Color _COLOR_LOG = Color(252, 191, 73, 255);
	const Color _COLOR_LOG_ERROR = Color(220, 50, 47, 255);
	const size_t _MAX_HISTORY = 512;

	bool _visible = false;
	bool _has_focus = false;
	bool _reclaim_focus = false;
	float _sleep_timer = 0.f;
	std::stringstream _cout_stream;
	std::stringstream _cerr_stream;
	std::string _command_line;
	std::deque<std::string> _command_queue; // TODO: use a ringbuffer instead
	std::deque<std::string> _command_history; // TODO: use a ringbuffer instead
	std::deque<std::string>::iterator _command_history_it = _command_history.end(); // TODO: use a ringbuffer instead
	std::deque<std::pair<std::string, Color>> _history; // TODO: use a ringbuffer instead
	std::unordered_map<window::Key, std::string> _key_bindings;

#ifdef _DEBUG_IMGUI
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
#endif

	void initialize() {
#if 0
		// REDIRECT COUT AND CERR

		std::cout.rdbuf(_cout_stream.rdbuf());
		std::cerr.rdbuf(_cerr_stream.rdbuf());
#endif

		// REGISTER COMMANDS

		register_commands();
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

#ifdef _DEBUG_IMGUI
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
			{
				float reserved_height = ImGui::GetStyle().ItemSpacing.y +
					ImGui::GetFrameHeightWithSpacing();
				if (ImGui::BeginChild(
					"History",
					ImVec2(0, -reserved_height), // Leave room for 1 separator + 1 input text
					false,
					ImGuiWindowFlags_HorizontalScrollbar)) {
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
					for (const auto& [line, color] : _history)
						ImGui::TextColored(ImColor(color.r, color.g, color.b, color.a), line.c_str());
					ImGui::PopStyleVar();
					if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
						ImGui::SetScrollHereY(1.0f); // Scroll to bottom
				}
				ImGui::EndChild();
			}

			ImGui::Separator();

			// COMMAND LINE

			ImGui::PushItemWidth(-1); // Use all available width
			if (ImGui::InputText(
				"##CommandLine",
				&_command_line,
				ImGuiInputTextFlags_EnterReturnsTrue |
				ImGuiInputTextFlags_CallbackCompletion |
				ImGuiInputTextFlags_CallbackHistory |
				ImGuiInputTextFlags_EscapeClearsAll,
				_input_text_callback)) {
				execute(_command_line);
				_command_line.clear();
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
#endif
	}

	void process_window_event(const window::Event& ev) {
		if (ev.type == window::EventType::KeyPress) {
			auto it = _key_bindings.find(ev.key.code);
			if (it != _key_bindings.end()) {
				execute(it->second);
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

	void log(std::string_view message) {
		_history.emplace_back(message, _COLOR_LOG);
		if (_history.size() > _MAX_HISTORY) {
			_history.pop_front();
		}
	}

	void log_error(std::string_view message, bool show_console) {
		_history.emplace_back(message, _COLOR_LOG_ERROR);
		if (_history.size() > _MAX_HISTORY) {
			_history.pop_front();
		}
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
		_history.emplace_back(command_line, _COLOR_COMMAND);
		if (_history.size() > _MAX_HISTORY) {
			_history.pop_front();
		}
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
		if (!filesystem::read_text_file(path, script)) {
			log_error("Failed to open console script: " + std::string(path));
			return;
		}
		std::istringstream script_stream(std::move(script));
		for (std::string line; std::getline(script_stream, line);) {
			execute(line, true);
		}
	}

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
}
