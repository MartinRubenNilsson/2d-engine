#pragma once

namespace audio {
	extern const std::string_view BUS_MASTER;
	extern const std::string_view BUS_SOUND;
	extern const std::string_view BUS_MUSIC;

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
	void stop_event(Handle<Event> handle);
	void set_event_volume(Handle<Event> handle, float volume);
	float get_event_volume(Handle<Event> handle);
	void set_event_position(Handle<Event> handle, const Vec2f& position);
	Vec2f get_event_position(Handle<Event> handle);
	void set_event_parameter_label(Handle<Event> handle, const std::string& name, const std::string& label);

	bool set_bus_volume(std::string_view bus_path, float volume);
	bool get_bus_volume(std::string_view bus_path, float& volume);
	bool stop_all_in_bus(std::string_view bus_path = BUS_MASTER);
}

