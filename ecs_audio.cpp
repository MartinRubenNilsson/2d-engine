#include "stdafx.h"
#include "ecs_audio.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void setup_audio_sources() {
		for (auto [entity, object] : _registry.view<Type<Tag::AudioSource>, ObjectId>().each()) {
			std::string_view event = get_string(object, "event");
			if (!event.empty()) {
				audio::create_event({
					.path = event.data(),
					.position = get_position(object) });
			}
		}
	}
}