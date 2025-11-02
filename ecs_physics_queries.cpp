#include "stdafx.h"
#include "ecs_physics.h"
#include "ecs_physics_queries.h"

namespace ecs {
	extern b2WorldId _physics_world;

	bool raycast_closest(const Vec2f& ray_start, const Vec2f& ray_end, uint32_t mask_bits, RaycastHit* hit) {
		b2QueryFilter query_filter = b2DefaultQueryFilter();
		query_filter.maskBits = mask_bits;

		const b2RayResult result = b2World_CastRayClosest(_physics_world, ray_start, ray_end - ray_start, query_filter);
		if (result.hit && hit) {
			hit->shape = result.shapeId;
			hit->body = b2Shape_GetBody(hit->shape);
			hit->entity = (entt::entity)(uintptr_t)b2Body_GetUserData(hit->body);
			hit->point = result.point;
			hit->normal = result.normal;
			hit->fraction = result.fraction;
		}

		return result.hit;
	}

	std::vector<RaycastHit> raycast(const Vec2f& ray_start, const Vec2f& ray_end, uint32_t mask_bits) {
		b2QueryFilter query_filter = b2DefaultQueryFilter();
		query_filter.maskBits = mask_bits;

		std::vector<RaycastHit> hits;

		b2World_CastRay(_physics_world, ray_start, ray_end - ray_start, query_filter,
			[](b2ShapeId shape_id, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
			RaycastHit hit{};
			hit.shape = shape_id;
			hit.body = b2Shape_GetBody(shape_id);
			hit.entity = (entt::entity)(uintptr_t)b2Body_GetUserData(hit.body);
			hit.point = point;
			hit.normal = normal;
			hit.fraction = fraction;
			((std::vector<RaycastHit>*)context)->push_back(hit);
			return 1.f;
		}, &hits);

		return hits;
	}

	std::vector<OverlapHit> overlap_box(const Rect2f& box, uint32_t mask_bits) {
		const b2AABB b2Box = { box.min, box.max };

		b2QueryFilter query_filter = b2DefaultQueryFilter();
		query_filter.maskBits = mask_bits;

		std::vector<OverlapHit> hits;
		b2World_OverlapAABB(_physics_world, b2Box, query_filter, [](b2ShapeId shape, void* context) {
			OverlapHit hit{};
			hit.shape = shape;
			hit.body = b2Shape_GetBody(shape);
			hit.entity = get_entity(hit.body);
			((std::vector<OverlapHit>*)context)->push_back(hit);
			return true;
		}, &hits);

		return hits;
	}

	std::vector<OverlapHit> overlap_circle(const Vec2f& center, float radius, uint32_t mask_bits) {
		const b2Circle circle = { center, radius };
		const b2ShapeProxy proxy = b2MakeProxy(&circle.center, 1, circle.radius);

		b2QueryFilter query_filter = b2DefaultQueryFilter();
		query_filter.maskBits = mask_bits;

		std::vector<OverlapHit> hits;
		b2World_OverlapShape(_physics_world, &proxy, query_filter,
			[](b2ShapeId shape_id, void* context) {
			OverlapHit hit{};
			hit.shape = shape_id;
			hit.body = b2Shape_GetBody(shape_id);
			hit.entity = get_entity(hit.body);
			((std::vector<OverlapHit>*)context)->push_back(hit);
			return true;
		}, &hits);

		return hits;
	}
}