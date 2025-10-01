#pragma once

namespace ecs {
	// Destroys the entity immediately. Safe to call on invalid entities.
	void destroy(entt::entity entity);
	void patch_entities_to_destroy(const struct Patch& patch);

	// Mark the entity to be destroyed at the end of the frame. Safe to call on invalid entities.
	void destroy_at_end_of_frame(entt::entity entity);
	void destroy_entities_to_be_destroyed_at_end_of_frame();
	void clear_entities_to_destroy_at_end_of_frame();

	void set_lifetime(entt::entity entity, float time);
	void update_lifetimes(float dt);
}
