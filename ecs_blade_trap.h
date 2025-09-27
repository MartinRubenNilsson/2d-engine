#pragma once

namespace ecs {
	struct BladeTrap {
		unsigned int update_count = 0;
		Vector2f direction;
		Vector2f start_position;
		Handle<audio::Event> audio_event;
	};

	void update_blade_traps(float dt);

	BladeTrap& emplace_blade_trap(entt::entity entity);
	BladeTrap* get_blade_trap(entt::entity entity);

	struct PhysicsEvent;
	void on_blade_trap_physics_event(const PhysicsEvent& ev);
}
