#include "stdafx.h"
#include "ecs_audio.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"
#include "audio.h"

namespace ecs {
	struct AudioListener {}; // Acts like a tag.

	extern entt::registry _registry;

	void set_audio_listener(entt::entity entity) {
		_registry.clear<AudioListener>(); // There can only be one audio listener at a time.
		_registry.emplace<AudioListener>(entity);
	}

	void _update_audio_listeners(float dt) {
		for (auto [entity, body] : _registry.view<AudioListener, b2BodyId>().each()) {
			const Vec2f pos = b2Body_GetWorldCenterOfMass(body);
			audio::set_listener_position(pos);
		}
	}

	void _setup_audio_sources() {
		for (auto [entity, object] : _registry.view<Type<Tag::AudioSource>, ObjectId>().each()) {
			std::string_view event = get_string(object, "event");
			if (!event.empty()) {
				audio::create_event({
					.path = event.data(),
					.position = get_position(object) });
			}
		}
	}

	void setup_audio() {
		_setup_audio_sources();
	}

	void update_audio(float dt) {
		_update_audio_listeners(dt);
	}
}