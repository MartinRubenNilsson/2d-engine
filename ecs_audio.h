#pragma once

namespace ecs {
	void set_audio_listener(entt::entity entity);

	void setup_audio();
	void update_audio(float dt);
}