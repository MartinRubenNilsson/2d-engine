#pragma once

namespace ecs
{
	struct RaycastHit
	{
		b2ShapeId shape = b2_nullShapeId;
		b2BodyId body = b2_nullBodyId;
		entt::entity entity = entt::null;
		Vector2f point;
		Vector2f normal;
		float fraction = 0.f; // 0 = start of ray, 1 = end of ray
	};

	struct OverlapHit
	{
		b2ShapeId shape = b2_nullShapeId;
		b2BodyId body = b2_nullBodyId;
		entt::entity entity = entt::null;
	};

	void initialize_physics();
	void shutdown_physics();
	void update_physics(float dt);
	void debug_draw_physics();

	bool raycast(const Vector2f& ray_start, const Vector2f& ray_end, uint32_t mask_bits = UINT32_MAX, RaycastHit* hit = nullptr);
	std::vector<RaycastHit> raycast_all(const Vector2f& ray_start, const Vector2f& ray_end, uint32_t mask_bits = UINT32_MAX);
	std::vector<OverlapHit> overlap_box(const Vector2f& box_min, const Vector2f& box_max, uint32_t mask_bits = UINT32_MAX);
	std::vector<OverlapHit> overlap_circle(const Vector2f& center, float radius, uint32_t mask_bits = UINT32_MAX);

	b2BodyId emplace_body(entt::entity entity, const b2BodyDef& body_def);
	b2BodyId deep_copy_and_emplace_body(entt::entity entity, b2BodyId body);
	b2BodyId get_body(entt::entity entity); // Returns b2_nullBodyId if entity has no body.
	bool remove_body(entt::entity entity);

	b2ShapeDef get_shape_def(b2ShapeId shape);
	b2BodyDef get_body_def(b2BodyId body);
	// Sets the category bits of all shapes attached to the body.
	void set_category_bits(b2BodyId body, uint32_t category_bits);
	// Returns the bitwise OR of all category bits of all shapes attached to the body.
	uint32_t get_category_bits(b2BodyId body);
}

