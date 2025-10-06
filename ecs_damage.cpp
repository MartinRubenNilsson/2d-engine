#include "stdafx.h"
#include "ecs_damage.h"
#include "ecs_physics_queries.h"

namespace ecs {

	extern entt::registry _registry;

	void set_damage_handler(entt::entity entity, DamageHandler handler) {
		if (handler) {
			_registry.emplace_or_replace<DamageHandler>(entity, handler);
		} else {
			_registry.remove<DamageHandler>(entity);
		}
	}

	DamageHandler get_damage_handler(entt::entity entity) {
		DamageHandler* handler_ptr = _registry.try_get<DamageHandler>(entity);
		return handler_ptr ? *handler_ptr : nullptr;
	}

	bool apply_damage(entt::entity entity, const Damage& damage) {
		if (entity == entt::null) return false;
		if (entity == damage.source) return false; // For now, entities can't damage themselves
		DamageHandler callback = get_damage_handler(entity);
		if (!callback) return false;
		return callback(entity, damage);
	}

	// This is used for record-keeping so we don't apply the same damage to the same entity multiple times.
	std::unordered_set<entt::entity> _entities_that_took_damage;

	bool apply_damage_in_box(const Damage& damage, const Vec2f& box_min, const Vec2f& box_max, uint32_t mask_bits) {
		//debug::draw_box(box_min, box_max, Color::Red, 0.2f);
		for (const OverlapHit& hit : overlap_box(box_min, box_max, mask_bits)) {
			if (hit.entity == damage.source) continue; // For now, entities can't damage themselves
			if (_entities_that_took_damage.contains(hit.entity)) continue;
			if (!apply_damage(hit.entity, damage)) continue;
			_entities_that_took_damage.insert(hit.entity);
		}
		const bool any_entity_took_damage = !_entities_that_took_damage.empty();
		_entities_that_took_damage.clear();
		return any_entity_took_damage;
	}

	bool apply_damage_in_circle(const Damage& damage, const Vec2f& center, float radius, uint32_t mask_bits) {
		//debug::draw_circle(center, radius, Color::Red, 0.2f);
		for (const OverlapHit& hit : overlap_circle(center, radius, mask_bits)) {
			if (hit.entity == damage.source) continue; // For now, entities can't damage themselves
			if (_entities_that_took_damage.contains(hit.entity)) continue;
			if (!apply_damage(hit.entity, damage)) continue;
			_entities_that_took_damage.insert(hit.entity);
		}
		const bool any_entity_took_damage = !_entities_that_took_damage.empty();
		_entities_that_took_damage.clear();
		return any_entity_took_damage;
	}
}