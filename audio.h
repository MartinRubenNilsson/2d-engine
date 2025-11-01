#pragma once

namespace audio {
	extern bool log_errors;

	void startup();
	void shutdown();
	void update();

	void load_bank(std::string_view path);

	void set_listener_position(const Vec2f& position);
	Vec2f get_listener_position();

	bool set_parameter(std::string_view name, float value);
	bool get_parameter(std::string_view name, float& value);
	// TODO: this doesn't work?
	bool set_parameter_label(const std::string& name, const std::string& label);
	bool get_parameter_label(const std::string& name, std::string& label);

	bool is_any_playing(std::string_view event_path);

	struct Event;

	struct EventOptions {
		float volume = 1.f;
		Vec2f position;
		bool start = true;
		bool release = true;
	};

	Handle<Event> create_event(std::string_view path, const EventOptions&& desc = {});
	bool valid(Handle<Event> event);
	void stop(Handle<Event> event);
	void set_volume(Handle<Event> event, float volume);
	float get_volume(Handle<Event> event);
	void set_position(Handle<Event> event, const Vec2f& position);
	Vec2f get_position(Handle<Event> event);
	void set_parameter_label(Handle<Event> event, const std::string& name, const std::string& label);

	extern const std::string_view BUS_MASTER;
	extern const std::string_view BUS_SOUND;
	extern const std::string_view BUS_MUSIC;

	void set_bus_volume(std::string_view bus_path, float volume);
	float get_bus_volume(std::string_view bus_path);
	void stop_all_in_bus(std::string_view bus_path = BUS_MASTER);
}

