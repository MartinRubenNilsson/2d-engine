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

	Vec2f _get_position_for_audio(entt::entity entity) {
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
		console::log_error("Error: Could not determine suitable audio position for entity = " + std::to_string((ENTT_ID_TYPE)entity));
		return Vec2f::ZERO;
	}

	void _update_audio_listeners() {
		for (auto [entity] : _registry.view<AudioListener>().each()) {
			const Vec2f position = _get_position_for_audio(entity);
			audio::set_listener_position(position);
		}
	}

	void _setup_audio_sources() {
		for (auto [entity, object] : _registry.view<Type<Tag::AudioSource>, ObjectId>().each()) {
			const std::string_view event_path = get_string(object, "event");
			if (event_path.empty())
				continue;
			play_attached_audio(event_path, entity);
		}
	}

	Handle<audio::Event> play_audio_at(std::string_view event_path, const Vec2f& position) {
		return audio::create_event(event_path, { .position = position });
	}

	Handle<audio::Event> play_audio_from(std::string_view event_path, entt::entity entity) {
		const Vec2f position = _get_position_for_audio(entity);
		return play_audio_at(event_path, position);
	}

	struct AttachedAudio {
		std::vector<Handle<audio::Event>> events;
	};

	Handle<audio::Event> play_attached_audio(std::string_view event_path, entt::entity entity) {
		if (!_registry.valid(entity))
			return {};
		const Vec2f position = _get_position_for_audio(entity);
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
			const Vec2f position = _get_position_for_audio(entity);
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

	void _on_destroy_attached_audio(entt::registry& registry, entt::entity entity) {
		AttachedAudio& attached = registry.get<AttachedAudio>(entity);
		for (Handle<audio::Event> event : attached.events) {
			audio::stop(event);
		}
	}

	void startup_audio() {
		_registry.on_destroy<AttachedAudio>().connect<_on_destroy_attached_audio>();
	}

	void shutdown_audio() {
		_registry.on_destroy<AttachedAudio>().disconnect<_on_destroy_attached_audio>();
	}

	void setup_audio() {
		_setup_audio_sources();
	}

	void update_audio() {
		_update_audio_listeners();
		_update_attached_audio();
	}
}