#pragma once

namespace audio {
	using ErrorMessageCallback = void(*)(std::string_view message);

	void set_error_message_callback(ErrorMessageCallback callback);

	bool startup();
	void shutdown();
	void update();

	void load_bank(std::string_view path);

	void set_listener_position(const Vec2f& position);
	Vec2f get_listener_position();

	void set_parameter(std::string_view name, float value);
	void get_parameter(std::string_view name, float& value);
	void set_parameter_label(std::string_view name, std::string_view label);
	// The returned string view is only valid until the next call to get_parameter_label().
	std::string_view get_parameter_label(std::string_view name);

	bool is_any_playing(std::string_view event_path);

	struct Event;

	struct EventOptions {
		float volume = 1.f;
		Vec2f position;
		bool start = true;
		bool release = true;
	};

	Handle<Event> create_event(std::string_view path, const EventOptions&& options = {});
	bool valid(Handle<Event> event);
	void stop(Handle<Event> event);
	void set_volume(Handle<Event> event, float volume);
	float get_volume(Handle<Event> event);
	void set_position(Handle<Event> event, const Vec2f& position);
	Vec2f get_position(Handle<Event> event);
	void set_parameter_label(Handle<Event> event, std::string_view name, std::string_view label);

	extern const std::string_view BUS_MASTER;
	extern const std::string_view BUS_SOUND;
	extern const std::string_view BUS_MUSIC;

	void set_bus_volume(std::string_view bus_path, float volume);
	float get_bus_volume(std::string_view bus_path);
	void stop_all_in_bus(std::string_view bus_path = BUS_MASTER);
}

