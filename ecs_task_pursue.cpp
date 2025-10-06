#include "stdafx.h"
#include "ecs_task_impl.h"
#include "ecs_physics.h"
#include "ecs_physics_queries.h"
#include "map_grid.h" // TODO: put this in ecs instead
#include "random.h"
#include "shapes.h"

namespace ecs {
	struct PursueTask {
		entt::entity target = entt::null;
		float speed = 0.f;
		float acceptance_radius = 0.f;
		bool pathfind = true;
		std::vector<Vec2i> path;
	};

	extern entt::registry _registry;

	void pursue(entt::entity entity, entt::entity target, float speed, float acceptance_radius, bool pathfind) {
		_registry.emplace_or_replace<Task>(entity, "pursue");
		_registry.emplace_or_replace<PursueTask>(entity, target, speed, acceptance_radius, pathfind);
	}

	// a = first point on arc
	// b = second point on arc
	// t = tangent at a, must be a unit vector
	Vec2f _parallel_transport_tangent_along_circular_arc(const Vec2f& a, const Vec2f& b, const Vec2f& t) {
		Vec2f s = b - a;
		float l = length(s);
		if (l < 0.0001f) // if the arc is degenerate
			return t;
		s /= l; // normalize
		// rotate the coordinate system so t is pointing in the positive x-direction
		s = complex_product(s, complex_conjugate(t));
		// parallel transport t in the rotated coordinate system
		s = complex_square(s);
		// rotate back
		s = complex_product(s, t);
		return s;
	}

	void _update_pursue_tasks(float dt) {
		for (auto [entity, task, pursue] : _registry.view<Task, PursueTask>().each()) {
			if (!_should(task, "pursue")) {
				_registry.erase<PursueTask>(entity);
				continue;
			}
			if (pursue.acceptance_radius <= 0.f ||
				!_registry.all_of<b2BodyId>(entity) ||
				!_registry.valid(pursue.target) ||
				!_registry.all_of<b2BodyId>(pursue.target)) {
				_registry.erase<PursueTask>(entity);
				continue;
			}
			if (task.status == TaskStatus::Preparing) {
				task.status = TaskStatus::Doing; // nothing to prepare
			}
			const b2BodyId body = _registry.get<b2BodyId>(entity);
			const b2BodyId target_body = _registry.get<b2BodyId>(pursue.target);
			const Vec2f pos = b2Body_GetWorldCenterOfMass(body);
			const Vec2f target_pos = b2Body_GetWorldCenterOfMass(target_body);
			const Vec2f to_target = target_pos - pos;
			const float dist_to_target = length(to_target);
			if (dist_to_target <= pursue.acceptance_radius) {
				task.status = TaskStatus::Succeeded;
				_registry.erase<PursueTask>(entity);
				continue;
			}
			Vec2f new_dir = to_target / dist_to_target;
			b2Body_SetLinearVelocity(body, new_dir * pursue.speed); // default: move directly towards target
			if (!pursue.pathfind) {
				pursue.path.clear();
				continue;
			}
			const uint32_t category_bits = get_category_bits(body);
			const uint32_t target_category_bits = get_category_bits(target_body);
			const uint32_t mask_bits = ~(category_bits | target_category_bits); // Exclude self and target.
			const Vec2f perp_dir = perp(new_dir);
			// If there's a direct line of sight, don't bother with pathfinding.
			if (!raycast_closest(pos + 8.f * perp_dir, target_pos, mask_bits) &&
				!raycast_closest(pos - 8.f * perp_dir, target_pos, mask_bits)) {
				pursue.path.clear();
				continue;
			}
			const Vec2i tile = map::world_to_tile(pos);
			const Vec2i target_tile = map::world_to_tile(target_pos);
			if (tile == target_tile)
				continue; // already on same tile as target
			if (!map::pathfind(tile, target_tile, pursue.path)) {
				task.status = TaskStatus::Failed; // cannot pathfind to target
				_registry.erase<PursueTask>(entity);
				continue;
			}
			if (pursue.path.size() < 2)
				continue; // defensive: already on same tile as target
			const Vec2i next_tile = pursue.path[1];
			const Vec2f next_pos = map::get_tile_center(next_tile);
			// the dir we want to be moving in once we're reached the next tile
			const Vec2f next_dir = normalize(next_tile - tile);
			// transport next_dir *backward* along the shortest circular arc to the next pos
			new_dir = -_parallel_transport_tangent_along_circular_arc(next_pos, pos, -next_dir);
			b2Body_SetLinearVelocity(body, new_dir * pursue.speed);
		}
	}

	void _debug_draw_pursue_tasks() {
		size_t paths_drawn = 0;
		for (auto [entity, pursue] : _registry.view<const PursueTask>().each()) {
			if (pursue.path.size() < 2)
				continue;
			const Vec2f offset = { paths_drawn * 2.f, paths_drawn * 2.f };
			for (size_t i = 0; i + 1 < pursue.path.size(); ++i) {
				const Vec2f p1 = map::get_tile_center(pursue.path[i]) + offset;
				const Vec2f p2 = map::get_tile_center(pursue.path[i + 1]) + offset;
				const Color color = random::color((uint32_t)entity * 2);
				shapes::add_line(p1, p2, color);
			}
			++paths_drawn;
		}
	}
}