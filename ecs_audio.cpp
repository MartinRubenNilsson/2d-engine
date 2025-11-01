#include "stdafx.h"
#include "ecs_audio.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"
#include "audio.h"
#include "sprites.h"
#include "console.h"

namespace ecs {
	struct AudioListener {}; // Acts like a tag.

	extern entt::registry _registry;

	void set_audio_listener(entt::entity entity) {
		_registry.clear<AudioListener>(); // There can only be one audio listener at a time.
		_registry.emplace<AudioListener>(entity);
	}

	Vec2f _determine_audio_position(entt::entity entity) {
		if (!_registry.valid(entity)) {
			console::log_error("Error: Trying to determine audio position for an invalid entity.");
			return Vec2f::ZERO;
		}
		// 1. Try the body center.
		if (const b2BodyId* body = _registry.try_get<b2BodyId>(entity)) {
			return b2Body_GetWorldCenterOfMass(*body);
		}
		// 2. Try the sprite center.
		if (const sprites::Sprite* sprite = _registry.try_get<sprites::Sprite>(entity)) {
			return sprite->position + sprite->size * 0.5f; // center of sprite
		}
		// 3. Try the object center.
		if (const ObjectId* object = _registry.try_get<ObjectId>(entity)) {
			return get_center(*object);
		}
		console::log_error("Error: Could not determine audio position for entity = " + std::to_string((ENTT_ID_TYPE)entity));
		return Vec2f::ZERO;
	}

	void _update_audio_listeners() {
		for (auto [entity] : _registry.view<AudioListener>().each()) {
			const Vec2f position = _determine_audio_position(entity);
			audio::set_listener_position(position);
		}
	}

	void _setup_audio_sources() {
		for (auto [entity, object] : _registry.view<Type<Tag::AudioSource>, ObjectId>().each()) {
			std::string_view event = get_string(object, "event");
			if (!event.empty()) {
				audio::create_event(event, { .position = get_position(object) });
			}
		}
	}

	Handle<audio::Event> play_audio_at(std::string_view event_path, const Vec2f& position) {
		return audio::create_event(event_path, { .position = position });
	}

	Handle<audio::Event> play_audio_at(std::string_view event_path, entt::entity entity) {
		const Vec2f position = _determine_audio_position(entity);
		return play_audio_at(event_path, position);
	}

	struct AttachedAudio {
		std::vector<Handle<audio::Event>> events;
	};

	Handle<audio::Event> play_attached_audio(std::string_view event_path, entt::entity entity) {
		if (!_registry.valid(entity))
			return {};
		const Vec2f position = _determine_audio_position(entity);
		const Handle<audio::Event> event = audio::create_event(event_path, { .position = position });
		if (event == Handle<audio::Event>())
			return event;
		AttachedAudio& attached = _registry.get_or_emplace<AttachedAudio>(entity);
		attached.events.push_back(event);
		return event;
	}

	void _update_attached_audio() {
		for (auto [entity, attached] : _registry.view<AttachedAudio>().each()) {
			if (attached.events.empty())
				continue;
			const Vec2f position = _determine_audio_position(entity);
			for (auto it = attached.events.begin(); it != attached.events.end();) {
				const Handle<audio::Event> event = *it;
				if (!audio::valid(event)) {
					it = attached.events.erase(it);
					continue;
				}
				audio::set_position(event, position);
				it++;
			}
		}
	}

	void setup_audio() {
		_setup_audio_sources();
	}

	void update_audio() {
		_update_audio_listeners();
		_update_attached_audio();
	}
}