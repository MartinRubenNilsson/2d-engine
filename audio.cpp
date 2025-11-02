#include "stdafx.h"
#include "audio.h"
#include "pool.h"
#include <fmod/fmod_studio.h>

#ifdef _DEBUG
#pragma comment(lib, "fmodL_vc.lib")
#pragma comment(lib, "fmodstudioL_vc.lib")
#else
#pragma comment(lib, "fmod_vc.lib")
#pragma comment(lib, "fmodstudio_vc.lib")
#endif

namespace audio {
	ErrorMessageCallback _error_message_callback = nullptr;

#define OUTPUT_ERROR_MESSAGE(message) { if (_error_message_callback) { _error_message_callback(message); } }

	void set_error_message_callback(ErrorMessageCallback callback) {
		_error_message_callback = callback;
	}

	FMOD_STUDIO_SYSTEM* _system = nullptr;

	bool startup() {
		FMOD_RESULT result = FMOD_Studio_System_Create(&_system, FMOD_VERSION);
		if (result != FMOD_OK)
			return false;
		constexpr int MAX_CHANNELS = 512;
		FMOD_STUDIO_INITFLAGS flags = 0;
#ifdef _DEBUG
		flags |= FMOD_STUDIO_INIT_LIVEUPDATE;
#endif
		result = FMOD_Studio_System_Initialize(_system, MAX_CHANNELS, flags, FMOD_INIT_NORMAL, nullptr);
		if (result != FMOD_OK)
			return false;
		return true;
	}

	void shutdown() {
		FMOD_Studio_System_Release(_system);
		_system = nullptr;
	}

	struct Event {
		FMOD_STUDIO_EVENTINSTANCE* instance = nullptr;
	};

	Pool<Event> _event_pool;
	std::unordered_set<size_t> _events_played_this_frame; // path hashes

	void update() {
		FMOD_Studio_System_Update(_system);
		_events_played_this_frame.clear();
	}

	const char* _c_str(std::string_view string) {
		const size_t size = string.size();
		constexpr size_t BUFFER_SIZE = 512;
		assert(size + 1 <= BUFFER_SIZE); // +1 to include null terminator
		static size_t offset = 0;
		if (offset + size + 1 > BUFFER_SIZE) {
			offset = 0;
		}
		static char buffer[BUFFER_SIZE];
		char* c_str = buffer + offset;
		memcpy(c_str, string.data(), size);
		c_str[size] = '\0';
		return c_str;
	}

	void load_bank_from_file(std::string_view path) {
		FMOD_STUDIO_BANK* bank = nullptr;
		const FMOD_RESULT result = FMOD_Studio_System_LoadBankFile(_system, _c_str(path), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
		if (result == FMOD_OK) return;
		OUTPUT_ERROR_MESSAGE("Failed to load audio bank: " + std::string(path))
	}

	const float _PIXELS_PER_FMOD_UNIT = 16.f;

	FMOD_3D_ATTRIBUTES _pos_to_3d_attributes(const Vec2f& position) {
		FMOD_3D_ATTRIBUTES attributes{};
		attributes.position.x = position.x / _PIXELS_PER_FMOD_UNIT;
		attributes.position.y = -position.y / _PIXELS_PER_FMOD_UNIT;
		attributes.position.z = 0.f;
		attributes.forward = { 0.f, 0.f, 1.f };
		attributes.up = { 0.f, 1.f, 0.f };
		return attributes;
	}

	Vec2f _3d_attributes_to_pos(const FMOD_3D_ATTRIBUTES& attributes) {
		return { attributes.position.x * _PIXELS_PER_FMOD_UNIT, -attributes.position.y * _PIXELS_PER_FMOD_UNIT };
	}

	void set_listener_position(const Vec2f& position) {
		FMOD_3D_ATTRIBUTES attributes = _pos_to_3d_attributes(position);
		FMOD_Studio_System_SetListenerAttributes(_system, 0, &attributes, nullptr);
	}

	Vec2f get_listener_position() {
		FMOD_3D_ATTRIBUTES attributes{};
		FMOD_Studio_System_GetListenerAttributes(_system, 0, &attributes, nullptr);
		return _3d_attributes_to_pos(attributes);
	}

	void set_parameter(std::string_view name, float value) {
		const FMOD_RESULT result = FMOD_Studio_System_SetParameterByName(_system, _c_str(name), value, false);
		if (result == FMOD_OK) return;
		OUTPUT_ERROR_MESSAGE("Could not find audio parameter: " + std::string(name) + "=" + std::to_string(value))
	}

	void get_parameter(std::string_view name, float& value) {
		const FMOD_RESULT result = FMOD_Studio_System_GetParameterByName(_system, _c_str(name), &value, nullptr);
		if (result == FMOD_OK) return;
		OUTPUT_ERROR_MESSAGE("Could not find audio parameter: " + std::string(name))
	}

	void set_parameter_label(std::string_view name, std::string_view label) {
		const FMOD_RESULT result = FMOD_Studio_System_SetParameterByNameWithLabel(_system, _c_str(name), _c_str(label), false);
		if (result == FMOD_OK) return;
		OUTPUT_ERROR_MESSAGE("Could not find audio parameter label: " + std::string(name) + "=" + std::string(label))
	}

	std::string_view get_parameter_label(std::string_view name) {
		float value = 0.f;
		FMOD_RESULT result = FMOD_Studio_System_GetParameterByName(_system, _c_str(name), &value, nullptr);
		if (result != FMOD_OK) {
			OUTPUT_ERROR_MESSAGE("Could not find audio parameter: " + std::string(name))
			return {};
		}
		static char buffer[256];
		int retrieved = 0; // including the null terminator
		result = FMOD_Studio_System_GetParameterLabelByName(_system, _c_str(name), (int)value, buffer, _countof(buffer), &retrieved);
		if (result != FMOD_OK) {
			OUTPUT_ERROR_MESSAGE("Could not get parameter label: " + std::string(name) + "=" + std::to_string(value))
			return {};
		}
		return { buffer, (size_t)retrieved - 1 }; // -1 to exclude the null terminator
	}

	FMOD_STUDIO_EVENTDESCRIPTION* _get_description(std::string_view event_path) {
		FMOD_STUDIO_EVENTDESCRIPTION* desc = nullptr;
		const FMOD_RESULT result = FMOD_Studio_System_GetEvent(_system, _c_str(event_path), &desc);
		if (result == FMOD_OK) return desc;
		OUTPUT_ERROR_MESSAGE("Could not find audio event: " + std::string(event_path))
		return nullptr;
	}

	// The returned span is valid only until the next call to _get_event_instances.
	std::span<FMOD_STUDIO_EVENTINSTANCE*> _get_all_instances(FMOD_STUDIO_EVENTDESCRIPTION* desc) {
		static FMOD_STUDIO_EVENTINSTANCE* buffer[1024];
		int count = 0;
		FMOD_Studio_EventDescription_GetInstanceList(desc, buffer, _countof(buffer), &count);
		return { buffer, (size_t)count };
	}

	bool is_any_playing(std::string_view event_path) {
		if (event_path.empty()) return false;
		FMOD_STUDIO_EVENTDESCRIPTION* desc = _get_description(_c_str(event_path));
		if (!desc) return false;
		for (FMOD_STUDIO_EVENTINSTANCE* instance : _get_all_instances(desc)) {
			FMOD_STUDIO_PLAYBACK_STATE state;
			FMOD_Studio_EventInstance_GetPlaybackState(instance, &state);
			if (state == FMOD_STUDIO_PLAYBACK_PLAYING) return true;
		}
		return false;
	}

	FMOD_STUDIO_EVENTINSTANCE* _create_instance(FMOD_STUDIO_EVENTDESCRIPTION* desc) {
		FMOD_STUDIO_EVENTINSTANCE* instance = nullptr;
		const FMOD_RESULT result = FMOD_Studio_EventDescription_CreateInstance(desc, &instance);
		if (result == FMOD_OK) return instance;
		OUTPUT_ERROR_MESSAGE("Failed to create audio event")
		return nullptr;
	}

	Handle<Event> _to_event_handle(void* userdata) {
		uint32_t uint = (uint32_t)(uintptr_t)userdata;
		return *(Handle<Event>*) & uint;
	}

	void* _to_userdata(Handle<Event> event) {
		uint32_t uint = *(uint32_t*)&event;
		return (void*)(uintptr_t)uint;
	}

	FMOD_RESULT F_CALL _on_event_destroyed(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* instance, void* parameters) {
		void* userdata = nullptr;
		FMOD_Studio_EventInstance_GetUserData(instance, &userdata);
		if (!userdata) return FMOD_OK;
		const Handle<Event> handle = _to_event_handle(userdata);
		_event_pool.free(handle);
		return FMOD_OK;
	}

	Handle<Event> create_event(std::string_view path, const EventOptions&& options) {
		if (path.empty()) return Handle<Event>();
		const size_t path_hash = std::hash<std::string_view>{}(path);
		if (_events_played_this_frame.contains(path_hash)) return Handle<Event>();
		FMOD_STUDIO_EVENTDESCRIPTION* studio_desc = _get_description(_c_str(path));
		if (!studio_desc) return Handle<Event>();
		FMOD_STUDIO_EVENTINSTANCE* instance = _create_instance(studio_desc);
		if (!instance) return Handle<Event>();
		Handle<Event> event = _event_pool.emplace(instance);
		_events_played_this_frame.insert(path_hash);
		FMOD_Studio_EventInstance_SetUserData(instance, _to_userdata(event));
		FMOD_Studio_EventInstance_SetCallback(instance, _on_event_destroyed, FMOD_STUDIO_EVENT_CALLBACK_DESTROYED);
		FMOD_Studio_EventInstance_SetVolume(instance, options.volume);
		FMOD_3D_ATTRIBUTES attributes = _pos_to_3d_attributes(options.position);
		FMOD_Studio_EventInstance_Set3DAttributes(instance, &attributes);
		if (options.start) {
			FMOD_Studio_EventInstance_Start(instance);
		}
		if (options.release) {
			FMOD_Studio_EventInstance_Release(instance);
		}
		return event;
	}

	bool valid(Handle<Event> event) {
		return _event_pool.valid(event);
	}

	FMOD_STUDIO_EVENTINSTANCE* _get_instance(Handle<Event> event) {
		Event* ev = _event_pool.get(event);
		if (!ev) return nullptr;
		return ev->instance;
	}

	void stop(Handle<Event> event) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_instance(event);
		if (!instance) return;
		FMOD_Studio_EventInstance_Stop(instance, FMOD_STUDIO_STOP_IMMEDIATE);
	}

	void set_volume(Handle<Event> event, float volume) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_instance(event);
		if (!instance) return;
		FMOD_Studio_EventInstance_SetVolume(instance, volume);
	}

	float get_volume(Handle<Event> event) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_instance(event);
		if (!instance) return 0.f;
		float volume = 0.f;
		FMOD_Studio_EventInstance_GetVolume(instance, &volume, nullptr);
		return volume;
	}

	void set_position(Handle<Event> event, const Vec2f& position) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_instance(event);
		if (!instance) return;
		FMOD_3D_ATTRIBUTES attributes = _pos_to_3d_attributes(position);
		FMOD_Studio_EventInstance_Set3DAttributes(instance, &attributes);
	}

	Vec2f get_position(Handle<Event> event) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_instance(event);
		if (!instance) return Vec2f::ZERO;
		FMOD_3D_ATTRIBUTES attributes{};
		FMOD_Studio_EventInstance_Get3DAttributes(instance, &attributes);
		return _3d_attributes_to_pos(attributes);
	}

	void set_parameter_label(Handle<Event> event, std::string_view name, std::string_view label) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_instance(event);
		if (!instance) return;
		FMOD_Studio_EventInstance_SetParameterByNameWithLabel(instance, _c_str(name), _c_str(label), false);
	}

	FMOD_STUDIO_BUS* _get_bus(std::string_view path) {
		FMOD_STUDIO_BUS* bus = nullptr;
		const FMOD_RESULT result = FMOD_Studio_System_GetBus(_system, _c_str(path), &bus);
		if (result == FMOD_OK) return bus;
		OUTPUT_ERROR_MESSAGE("Could not find audio bus: " + std::string(path))
		return bus;
	}

	const std::string_view BUS_MASTER = "bus:/";
	const std::string_view BUS_SOUND = "bus:/sound";
	const std::string_view BUS_MUSIC = "bus:/music";

	void set_bus_volume(std::string_view bus_path, float volume) {
		FMOD_STUDIO_BUS* bus = _get_bus(bus_path);
		if (!bus) return;
		FMOD_Studio_Bus_SetVolume(bus, volume);
	}

	float get_bus_volume(std::string_view bus_path) {
		FMOD_STUDIO_BUS* bus = _get_bus(bus_path);
		if (!bus) return 0.f;
		float volume = 0.f;
		FMOD_Studio_Bus_GetVolume(bus, &volume, nullptr);
		return volume;
	}

	void stop_all_in_bus(std::string_view bus_path) {
		FMOD_STUDIO_BUS* bus = _get_bus(bus_path);
		if (!bus) return;
		FMOD_Studio_Bus_StopAllEvents(bus, FMOD_STUDIO_STOP_IMMEDIATE);
	}
}
