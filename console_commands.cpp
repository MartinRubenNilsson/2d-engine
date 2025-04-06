#include "stdafx.h"
#include "console.h"
#include "console_commands.h"

namespace console {

	bool get_bool(const Arg& arg) {
		if (std::holds_alternative<bool>(arg)) return std::get<bool>(arg);
		log_error("Invalid argument type: expected bool");
		return false;
	}

	bool get_int(const Arg& arg) {
		if (std::holds_alternative<int>(arg)) return std::get<int>(arg);
		log_error("Invalid argument type: expected int");
		return 0;
	}

	bool get_float(const Arg& arg) {
		if (std::holds_alternative<float>(arg)) return std::get<float>(arg);
		log_error("Invalid argument type: expected float");
		return 0.f;
	}

	std::string get_string(const Arg& arg) {
		if (std::holds_alternative<std::string>(arg)) return std::get<std::string>(arg);
		log_error("Invalid argument type: expected string");
		return "";
	}

	Vector2f get_vector2f(const Arg& arg) {
		if (std::holds_alternative<Vector2f>(arg)) return std::get<Vector2f>(arg);
		log_error("Invalid argument type: expected Vector2f");
		return Vector2f{};
	}

	std::string_view _command_param_type_to_string(ParamType type) {
		switch (type) {
		case ParamType::None:     return "NONE";
		case ParamType::Bool:     return "BOOL";
		case ParamType::Int:      return "INT";
		case ParamType::Float:    return "FLOAT";
		case ParamType::String:   return "STRING";
		case ParamType::Vector2f: return "VEC2F";
		default:                  return "UNKNOWN";
		}
	}

	std::string _format_command_param(const Param& param) {
		std::string ret;
		ret += _command_param_type_to_string(param.type);
		ret += " ";
		ret += param.name;
		return ret;
	}

	std::string format_command_help_message(const Command& command) {
		std::string ret;
		ret += command.name;
		for (const Param& param : command.params) {
			if (param.type == ParamType::None) break; // End of parameters
			ret += " [" + _format_command_param(param) + "]";
		}
		ret += "\n- ";
		ret += command.desc;
		for (const Param& param : command.params) {
			if (param.type == ParamType::None) break; // End of parameters
			ret += "\n- ";
			ret += param.name;
			ret += ": ";
			ret += param.desc;
		}
		return ret;
	}

	std::vector<Command> _commands; // Sorted by name

	bool operator<(const Command& left, const Command& right) {
		return left.name < right.name; // Order by name
	}

	void clear_commands() {
		_commands.clear();
	}

	void add_command(const Command&& command) {
		_commands.emplace_back(command);
	}

	void sort_commands_by_name() {
		std::sort(_commands.begin(), _commands.end()); // Sort commands by name
	}

	const Command* find_command_with_name(std::string_view name) {
		// Do a binary search to find the command. We assume the commands are sorted by name.
		auto it = std::lower_bound(_commands.begin(), _commands.end(), Command{ name });
		if (it == _commands.end()) return nullptr;
		if (it->name != name) return nullptr;
		return &(*it);
	}

	std::span<const Command> find_commands_whose_name_starts_with(std::string_view prefix) {
		const size_t prefix_size = prefix.size();
		auto [begin, end] = std::equal_range(_commands.begin(), _commands.end(), Command{ prefix },
			[prefix_size](const Command& left, const Command& right) {
			return strncmp(left.name.data(), right.name.data(), prefix_size) < 0;
		});
		return { begin, end };
	}

	Arg _get_default_arg_for_param_type(ParamType type) {
		switch (type) {
		case ParamType::None:     return std::monostate{};
		case ParamType::Bool:     return false;
		case ParamType::Int:      return 0;
		case ParamType::Float:    return 0.f;
		case ParamType::String:   return std::string{};
		case ParamType::Vector2f: return Vector2f{};
		default:                  return std::monostate{};
		}
	}

	struct ArgParserVisitor {
		std::istream& is;

		void operator()(std::monostate&) {}
		void operator()(bool& value) { is >> std::boolalpha >> value >> std::noboolalpha; }
		void operator()(int& value) { is >> value; }
		void operator()(float& value) { is >> value; }
		void operator()(std::string& value) {
			// Try to parse a string with quotes. If it fails, parse a string without quotes.
			if (is.peek() == '"') {
				is.ignore();
				std::getline(is, value, '"');
			} else {
				is >> value;
			}
		}
		void operator()(Vector2f& value) { is >> value.x >> value.y; }
	};

	void parse_and_execute_command(std::string_view command_line) {
		std::istringstream iss(std::string{ command_line });
		std::string name;
		if (!(iss >> name)) return;
		const Command* command = find_command_with_name(name);
		if (!command) {
			log_error("Unknown command: " + std::string(name));
			return;
		}
		if (!command->callback) {
			log_error("Command is missing a callback: " + name);
			return;
		}
		ArgList args{};
		for (size_t i = 0; i < MAX_PARAMS; ++i) {
			const Param& param = command->params[i];
			if (param.type == ParamType::None) break; // End of parameters
			iss.ignore(64, ' '); // Skip any leading spaces
			if (iss.eof()) {
				log_error("Missing argument: " + _format_command_param(param));
				return;
			}
			Arg& arg = args[i];
			arg = _get_default_arg_for_param_type(param.type); // Ensure the arg is initialized with correct type
			std::visit(ArgParserVisitor{ iss }, arg);
			if (iss.fail()) {
				log_error("Invalid argument: " + _format_command_param(param));
				return;
			}
		}
		command->callback(args); // Execute the command
	}

	void register_commands() {
		clear_commands();
		_add_misc_commands();
		_add_steam_commands();
		sort_commands_by_name();
	}
}
