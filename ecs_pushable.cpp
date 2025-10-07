#include "stdafx.h"
#include "ecs_pushable.h"
#include "ecs_tags.h"
#include "ecs_physics_events.h"
#include "audio.h"

namespace ecs {
	struct PushableBlock {
		Handle<audio::Event> stone_sliding_sound{};
	};

	extern entt::registry _registry;

	void _handle_physics_event_for_pushable_block(const PhysicsEvent& ev) {
		const Tag other_tag = get_tag(ev.other_entity);
		if (other_tag != Tag::Player)
			return;
		PushableBlock& pushable = _registry.get<PushableBlock>(ev.entity);
		if (ev.type == PhysicsEventType::ContactBeginTouch) {
			b2Body_SetType(ev.body, b2_dynamicBody);
			audio::stop_event(pushable.stone_sliding_sound); // defensive
			pushable.stone_sliding_sound = audio::create_event({ .path = "event:/props/stone_slide" });
		} else if (ev.type == PhysicsEventType::ContactEndTouch) {
			b2Body_SetType(ev.body, b2_staticBody);
			audio::stop_event(pushable.stone_sliding_sound);
		}
	}

	void setup_pushables() {
		for (auto [entity] : _registry.view<Type<Tag::PushableBlock>>().each()) {
			_registry.emplace<PushableBlock>(entity);
			set_physics_event_handler(entity, _handle_physics_event_for_pushable_block);
		}
	}
}
