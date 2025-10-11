#pragma once

namespace audio
{
	struct Event;

	extern const std::string BUS_MASTER;
	extern const std::string BUS_SOUND;
	extern const std::string BUS_MUSIC;

	extern bool log_errors;

	void initialize();
	void shutdown();
	void update();
	void load_bank_from_file(const std::string& path);

	// LISTENERS

	void set_listener_position(const Vec2f& position);
	Vec2f get_listener_position();

	// GLOBAL PARAMETERS

	bool set_parameter(const std::string& name, float value);
	bool get_parameter(const std::string& name, float& value);
	// TODO: this doesn't work?
	bool set_parameter_label(const std::string& name, const std::string& label);
	bool get_parameter_label(const std::string& name, std::string& label);

	// EVENTS

	bool is_any_playing(const std::string &event_path);

	struct EventDesc {
		std::string path;
		float volume = 1.f;
		Vec2f position;
		bool start = true;
		bool release = true;
	};

	Handle<Event> create_event(const EventDesc&& desc);
	void stop_event(Handle<Event> handle);
	void set_event_volume(Handle<Event> handle, float volume);
	float get_event_volume(Handle<Event> handle);
	void set_event_position(Handle<Event> handle, const Vec2f& position);
	Vec2f get_event_position(Handle<Event> handle);
	void set_event_parameter_label(Handle<Event> handle, const std::string& name, const std::string& label);

	// BUSES

	bool set_bus_volume(const std::string& bus_path, float volume);
	bool get_bus_volume(const std::string& bus_path, float& volume);
	bool stop_all_in_bus(const std::string& bus_path = BUS_MASTER);
}

