#pragma once

namespace ecs {
	void initialize_blade_traps();
	void update_blade_traps(float dt);

	struct PhysicsEvent;
	void on_blade_trap_physics_event(const PhysicsEvent& ev);
}
