#include "stdafx.h"
#include "audio.h"
#include "pool.h"
#include "console.h"
#include <fmod/fmod_studio.h>

#ifdef _DEBUG
#pragma comment(lib, "fmodL_vc.lib")
#pragma comment(lib, "fmodstudioL_vc.lib")
#else
#pragma comment(lib, "fmod_vc.lib")
#pragma comment(lib, "fmodstudio_vc.lib")
#endif

namespace audio {
	const std::string BUS_MASTER = "bus:/";
	const std::string BUS_SOUND = "bus:/sound";
	const std::string BUS_MUSIC = "bus:/music";
	const float _PIXELS_PER_FMOD_UNIT = 16.f;

	bool log_errors =
#ifdef _DEBUG
		true;
#else
		false;
#endif

	struct Event {
		FMOD_STUDIO_EVENTINSTANCE* instance = nullptr;
	};

	FMOD_STUDIO_SYSTEM* _system = nullptr;
	FMOD_STUDIO_EVENTINSTANCE* _event_buffer[1024] = {};
	Pool<Event> _event_pool;
	std::unordered_set<std::string> _events_played_this_frame;

	void* _event_handle_to_userdata(Handle<Event> handle) {
		uint32_t uint = *(uint32_t*)&handle;
		return (void*)(uintptr_t)uint;
	}

	Handle<Event> _userdata_to_event_handle(void* userdata) {
		uint32_t uint = (uint32_t)(uintptr_t)userdata;
		return *(Handle<Event>*) & uint;
	}

	FMOD_RESULT F_CALL _fmod_callback_on_event_destroyed(
		FMOD_STUDIO_EVENT_CALLBACK_TYPE type,
		FMOD_STUDIO_EVENTINSTANCE* instance,
		void* parameters) {
		void* userdata = nullptr;
		FMOD_Studio_EventInstance_GetUserData(instance, &userdata);
		if (!userdata) return FMOD_OK;
		_event_pool.free(_userdata_to_event_handle(userdata));
		return FMOD_OK;
	}

	FMOD_STUDIO_BUS* _get_bus(const std::string& path) {
		FMOD_STUDIO_BUS* bus = nullptr;
		FMOD_RESULT result = FMOD_Studio_System_GetBus(_system, path.c_str(), &bus);
		if (result != FMOD_OK && log_errors) {
			console::log_error("Could not find audio bus: " + path);
		}
		return bus;
	}

	FMOD_STUDIO_EVENTDESCRIPTION* _get_event_description(const char* path) {
		FMOD_STUDIO_EVENTDESCRIPTION* desc = nullptr;
		FMOD_RESULT result = FMOD_Studio_System_GetEvent(_system, path, &desc);
		if (result != FMOD_OK && log_errors) {
			console::log_error("Could not find audio event: " + std::string(path));
		}
		return desc;
	}

	FMOD_STUDIO_EVENTINSTANCE* _create_event_instance(FMOD_STUDIO_EVENTDESCRIPTION* desc) {
		FMOD_STUDIO_EVENTINSTANCE* instance = nullptr;
		FMOD_RESULT result = FMOD_Studio_EventDescription_CreateInstance(desc, &instance);
		if (result != FMOD_OK && log_errors) {
			console::log_error("Failed to create audio event");
		}
		return instance;
	}

	// The returned span is valid only until the next call to _get_event_instances.
	std::span<FMOD_STUDIO_EVENTINSTANCE*> _get_event_instances(FMOD_STUDIO_EVENTDESCRIPTION* desc) {
		int count = 0;
		FMOD_Studio_EventDescription_GetInstanceList(desc, _event_buffer, _countof(_event_buffer), &count);
		return std::span(_event_buffer, count);
	}

	FMOD_STUDIO_EVENTINSTANCE* _get_event_instance(Handle<Event> handle) {
		Event* ev = _event_pool.get(handle);
		if (!ev) return nullptr;
		return ev->instance;
	}

	void initialize() {
		FMOD_RESULT result = FMOD_Studio_System_Create(&_system, FMOD_VERSION);
		assert(result == FMOD_OK);
		FMOD_STUDIO_INITFLAGS flags = 0;
#ifdef _DEBUG
		flags |= FMOD_STUDIO_INIT_LIVEUPDATE;
#endif
		result = FMOD_Studio_System_Initialize(
			_system,
			512, // max channels
			flags,
			FMOD_INIT_NORMAL,
			nullptr);
		assert(result == FMOD_OK);
	}

	void shutdown() {
		FMOD_Studio_System_Release(_system);
		_system = nullptr;
	}

	void update() {
		FMOD_Studio_System_Update(_system);
		_events_played_this_frame.clear();
	}

	void load_bank_from_file(const std::string& path) {
		FMOD_STUDIO_BANK* bank = nullptr;
		FMOD_RESULT result = FMOD_Studio_System_LoadBankFile(
			_system, path.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
		if (result != FMOD_OK && log_errors) {
			console::log_error("Failed to load audio bank: " + path);
		}
	}

	FMOD_3D_ATTRIBUTES _pos_to_3d_attributes(const Vec2f& position) {
		FMOD_3D_ATTRIBUTES attributes{};
		attributes.position = { position.x / _PIXELS_PER_FMOD_UNIT, -position.y / _PIXELS_PER_FMOD_UNIT, 0.f };
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

	bool set_parameter(const std::string& name, float value) {
		FMOD_RESULT result = FMOD_Studio_System_SetParameterByName(_system, name.c_str(), value, false);
		if (result == FMOD_OK) return true;
		if (log_errors) {
			console::log_error("Could not find audio parameter: " + name + "=" + std::to_string(value));
		}
		return false;
	}

	bool get_parameter(const std::string& name, float& value) {
		FMOD_RESULT result = FMOD_Studio_System_GetParameterByName(_system, name.c_str(), &value, nullptr);
		if (result == FMOD_OK) return true;
		if (log_errors) {
			console::log_error("Could not find audio parameter: " + name);
		}
		return false;
	}

	bool set_parameter_label(const std::string& name, const std::string& label) {
		FMOD_RESULT result = FMOD_Studio_System_SetParameterByNameWithLabel(_system, name.c_str(), label.c_str(), false);
		if (result == FMOD_OK) return true;
		if (log_errors) {
			console::log_error("Could not find audio parameter label: " + name + "=" + label);
		}
		return false;
	}

	bool get_parameter_label(const std::string& name, std::string& label) {
		float value = 0.f;
		FMOD_RESULT result = FMOD_Studio_System_GetParameterByName(_system, name.c_str(), &value, nullptr);
		if (result != FMOD_OK) {
			if (log_errors) {
				console::log_error("Could not find audio parameter: " + name);
			}
			return false;
		}
		char label_buffer[256];
		result = FMOD_Studio_System_GetParameterLabelByName(
			_system, name.c_str(), (int)value, label_buffer, _countof(label_buffer), nullptr);
		if (result != FMOD_OK) {
			if (log_errors) {
				console::log_error("Could not get parameter label: " + name + "=" + std::to_string(value));
			}
		}
		label = label_buffer;
		return true;
	}

	bool is_any_playing(const std::string& event_path) {
		if (event_path.empty()) return false;
		FMOD_STUDIO_EVENTDESCRIPTION* desc = _get_event_description(event_path.c_str());
		if (!desc) return false;
		for (FMOD_STUDIO_EVENTINSTANCE* instance : _get_event_instances(desc)) {
			FMOD_STUDIO_PLAYBACK_STATE state;
			FMOD_Studio_EventInstance_GetPlaybackState(instance, &state);
			if (state == FMOD_STUDIO_PLAYBACK_PLAYING) return true;
		}
		return false;
	}

	Handle<Event> create_event(const EventDesc&& desc) {
		if (desc.path.empty()) return Handle<Event>();
		if (_events_played_this_frame.contains(desc.path)) return Handle<Event>();
		FMOD_STUDIO_EVENTDESCRIPTION* studio_desc = _get_event_description(desc.path.c_str());
		if (!studio_desc) return Handle<Event>();
		FMOD_STUDIO_EVENTINSTANCE* instance = _create_event_instance(studio_desc);
		if (!instance) return Handle<Event>();
		Handle<Event> handle = _event_pool.emplace(Event{ .instance = instance });
		_events_played_this_frame.insert(desc.path);
		FMOD_Studio_EventInstance_SetCallback(instance, _fmod_callback_on_event_destroyed, FMOD_STUDIO_EVENT_CALLBACK_DESTROYED);
		FMOD_Studio_EventInstance_SetUserData(instance, _event_handle_to_userdata(handle));
		FMOD_Studio_EventInstance_SetVolume(instance, desc.volume);
		FMOD_3D_ATTRIBUTES attributes = _pos_to_3d_attributes(desc.position);
		FMOD_Studio_EventInstance_Set3DAttributes(instance, &attributes);
		if (desc.start) {
			FMOD_Studio_EventInstance_Start(instance);
		}
		if (desc.release) {
			FMOD_Studio_EventInstance_Release(instance);
		}
		return handle;
	}

	bool stop_event(Handle<Event> handle) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_event_instance(handle);
		if (!instance) return false;
		FMOD_Studio_EventInstance_Stop(instance, FMOD_STUDIO_STOP_IMMEDIATE);
		return true;
	}

	bool set_event_volume(Handle<Event> handle, float volume) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_event_instance(handle);
		if (!instance) return false;
		FMOD_Studio_EventInstance_SetVolume(instance, volume);
		return true;
	}

	bool get_event_volume(Handle<Event> handle, float& volume) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_event_instance(handle);
		if (!instance) return false;
		FMOD_Studio_EventInstance_GetVolume(instance, &volume, nullptr);
		return true;
	}

	bool set_event_position(Handle<Event> handle, const Vec2f& position) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_event_instance(handle);
		if (!instance) return false;
		FMOD_3D_ATTRIBUTES attributes = _pos_to_3d_attributes(position);
		FMOD_Studio_EventInstance_Set3DAttributes(instance, &attributes);
		return true;
	}

	bool get_event_position(Handle<Event> handle, Vec2f& position) {
		FMOD_STUDIO_EVENTINSTANCE* instance = _get_event_instance(handle);
		if (!instance) return false;
		FMOD_3D_ATTRIBUTES attributes{};
		FMOD_Studio_EventInstance_Get3DAttributes(instance, &attributes);
		position = _3d_attributes_to_pos(attributes);
		return true;
	}

	bool set_bus_volume(const std::string& bus_path, float volume) {
		FMOD_STUDIO_BUS* bus = _get_bus(bus_path);
		if (!bus) return false;
		FMOD_Studio_Bus_SetVolume(bus, volume);
		return true;
	}

	bool get_bus_volume(const std::string& bus_path, float& volume) {
		FMOD_STUDIO_BUS* bus = _get_bus(bus_path);
		if (!bus) return false;
		FMOD_Studio_Bus_GetVolume(bus, &volume, nullptr);
		return true;
	}

	bool stop_all_in_bus(const std::string& bus_path) {
		FMOD_STUDIO_BUS* bus = _get_bus(bus_path);
		if (!bus) return false;
		FMOD_Studio_Bus_StopAllEvents(bus, FMOD_STUDIO_STOP_IMMEDIATE);
		return true;
	}
}
