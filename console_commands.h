#pragma once

namespace console {

	enum class ParamType {
		None,
		Bool,
		Int,
		Float,
		String,
		Vec2f
	};

	struct Param {
		ParamType type = ParamType::None;
		std::string_view name;
		std::string_view desc;
	};

	using Arg = std::variant<
		std::monostate, // None
		bool,
		int,
		float,
		std::string,
		Vec2f
	>;

	// Please use these instead of std::get<> so we can avoid exceptions.

	bool get_bool(const Arg& arg);
	bool get_int(const Arg& arg);
	bool get_float(const Arg& arg);
	std::string get_string(const Arg& arg);
	Vec2f get_vector2f(const Arg& arg);

	constexpr size_t MAX_PARAMS = 8; // Maximum number of parameters for a command
	using ParamList = std::array<Param, MAX_PARAMS>;
	using ArgList = std::array<Arg, MAX_PARAMS>;

	struct Command {
		std::string_view name;
		std::string_view desc;
		ParamList params{};
		void (*callback)(const ArgList& args) = nullptr;
	};

	std::string format_command_help_message(const Command& command);

	void clear_commands();
	void add_command(const Command&& command);
	// Needs to be called after adding commands but before searching.
	void sort_commands_by_name();

	const Command* find_command_with_name(std::string_view name);
	std::span<const Command> find_commands_whose_name_starts_with(std::string_view prefix);

	void parse_and_execute_command(std::string_view command_line);

	// Call once at engine startup.
	void register_commands();
	void _add_misc_commands(); // console_commands_misc.cpp
	void _add_steam_commands(); // console_commands_steam.cpp
}