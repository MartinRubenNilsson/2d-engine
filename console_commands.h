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
	Vec2f get_vec2f(const Arg& arg);

	using Args = std::span<const Arg>;

	struct Command {
		std::string_view name;
		std::string_view desc;
		std::vector<Param> params;
		void (*callback)(Args args) = nullptr;
	};

	std::string get_command_help_message(const Command& command);
	
	void register_command(const Command&& command);

	const Command* find_command_with_name(std::string_view name);
	std::span<const Command> find_commands_whose_name_starts_with(std::string_view prefix);

	void parse_and_execute_command(std::string_view command_line);
	
	void register_commands(); // Call once at engine startup.
}