#pragma once

namespace ecs {
	void set_audio_listener(entt::entity entity);

	Handle<audio::Event> play_audio_at(std::string_view event_path, const Vec2f& position);
	Handle<audio::Event> play_audio_at(std::string_view event_path, entt::entity entity);
	Handle<audio::Event> play_attached_audio(std::string_view event_path, entt::entity entity);

	void setup_audio();
	void update_audio();
}