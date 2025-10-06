#pragma once

namespace ecs {
	void startup_physics();
	void shutdown_physics();

	void setup_physics(MapId map);
	void update_physics(float dt);
	void debug_draw_physics();

	b2ShapeDef get_shape_def(b2ShapeId shape);
	b2BodyDef get_body_def(b2BodyId body);
	// Returns the bitwise OR of all category bits of all shapes attached to the body.
	uint32_t get_category_bits(b2BodyId body);

	b2BodyId emplace_body(entt::entity entity, const b2BodyDef& body_def);
	b2BodyId deep_copy_and_emplace_body(entt::entity entity, b2BodyId body);
	b2BodyId get_body(entt::entity entity); // Returns b2_nullBodyId if entity has no body.
	bool remove_body(entt::entity entity);
}
