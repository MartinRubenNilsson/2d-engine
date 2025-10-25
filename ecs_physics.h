#pragma once

namespace ecs {
	void startup_physics();
	void shutdown_physics();

	void setup_physics(MapId map);
	void clear_physics();
	void update_physics(float dt);
	void debug_draw_physics(const Rect2f& view);

	// Swaps the role of shapeA and shapeB in the manifold. (Inverts the normal vector, etc.)
	void swap(b2Manifold& manifold);
	// Swaps the role of shapeA and shapeB in the contact. (Inverts the normal vector, etc.)
	void swap(b2ContactData& contact);
	
	b2BodyId emplace_body(entt::entity entity, const b2BodyDef& def);
	b2BodyId get_body(entt::entity entity); // Returns b2_nullBodyId if entity has no body.

	entt::entity get_entity(b2BodyId body);
	b2BodyDef get_def(b2BodyId body);
	// WARNING: The returned span is only valid until the next call to get_shapes()!
	std::span<const b2ShapeId> get_shapes(b2BodyId body);
	// Returns all contacts for the body, with shapeIdA guaranteed to belong to the body.
	// WARNING: The returned span is only valid until the next call to get_contacts()!
	std::span<const b2ContactData> get_contacts(b2BodyId body);
	// Returns the bitwise OR of all category bits of all shapes attached to the body.
	uint32_t get_category_bits(b2BodyId body);

	void remove_body(entt::entity entity);
	void stop_moving(entt::entity entity); // Sets the velocity to zero.
}
