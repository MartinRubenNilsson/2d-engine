#pragma once

namespace ecs {
	// Destroys the entity immediately. Safe to call on invalid entities.
	void destroy_now(entt::entity entity);
	void patch_entities_to_destroy(const struct Patch& patch);

	// Mark the entity to be destroyed at the end of the logic frame. Safe to call on invalid entities.
	void destroy_later(entt::entity entity);
	void destroy_entities_to_be_destroyed_later();
	void clear_entities_to_be_destroyed_later();

	void set_lifetime(entt::entity entity, float time);
	void update_lifetimes(float dt);
}
