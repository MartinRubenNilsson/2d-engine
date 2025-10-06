#pragma once

namespace ecs {
	struct RaycastHit {
		b2ShapeId shape = b2_nullShapeId;
		b2BodyId body = b2_nullBodyId;
		entt::entity entity = entt::null;
		Vec2f point;
		Vec2f normal;
		float fraction = 0.f; // 0 = start of ray, 1 = end of ray
	};

	bool raycast_closest(const Vec2f& ray_start, const Vec2f& ray_end, uint32_t mask_bits = UINT32_MAX, RaycastHit* hit = nullptr);
	std::vector<RaycastHit> raycast(const Vec2f& ray_start, const Vec2f& ray_end, uint32_t mask_bits = UINT32_MAX);

	struct OverlapHit {
		b2ShapeId shape = b2_nullShapeId;
		b2BodyId body = b2_nullBodyId;
		entt::entity entity = entt::null;
	};

	std::vector<OverlapHit> overlap_box(const Vec2f& box_min, const Vec2f& box_max, uint32_t mask_bits = UINT32_MAX);
	std::vector<OverlapHit> overlap_circle(const Vec2f& center, float radius, uint32_t mask_bits = UINT32_MAX);
}