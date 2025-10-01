#include "stdafx.h"
#include "ecs_audio.h"
#include "ecs_tags.h"
#include "ecs_tiled.h"
#include "audio.h"

namespace ecs {
	extern entt::registry _registry;

	void setup_audio_sources() {
		for (auto [entity, object] : _registry.view<Type<Tag::AudioSource>, TObject>().each()) {
			std::string_view event = object.get_string("event");
			if (!event.empty()) {
				audio::create_event({
					.path = event.data(),
					.position = object.get_position() });
			}
		}
	}
}