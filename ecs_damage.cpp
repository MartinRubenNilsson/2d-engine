#include "stdafx.h"
#include "ecs_damage.h"
#include "ecs_physics_queries.h"

namespace ecs {

	extern entt::registry _registry;

	void set_damage_event_handler(entt::entity entity, DamageEventHandler handler) {
		if (handler) {
			_registry.emplace_or_replace<DamageEventHandler>(entity, handler);
		} else {
			_registry.remove<DamageEventHandler>(entity);
		}
	}

	DamageEventHandler get_damage_event_handler(entt::entity entity) {
		DamageEventHandler* handler_ptr = _registry.try_get<DamageEventHandler>(entity);
		return handler_ptr ? *handler_ptr : nullptr;
	}

	bool deal_damage(entt::entity entity, const DamageEvent& ev) {
		if (entity == entt::null) return false;
		if (entity == ev.source) return false; // For now, entities can't damage themselves
		DamageEventHandler callback = get_damage_event_handler(entity);
		if (!callback) return false;
		return callback(entity, ev);
	}

	// This is used for record-keeping so we don't apply the same damage to the same entity multiple times.
	std::unordered_set<entt::entity> _entities_that_took_damage;

	bool deal_damage_in_box(const DamageEvent& ev, const Rect2f& box, uint32_t mask_bits) {
		//debug::draw_box(box_min, box_max, Color::Red, 0.2f);
		for (const OverlapHit& hit : overlap_box(box, mask_bits)) {
			if (hit.entity == ev.source) continue; // For now, entities can't damage themselves
			if (_entities_that_took_damage.contains(hit.entity)) continue;
			if (!deal_damage(hit.entity, ev)) continue;
			_entities_that_took_damage.insert(hit.entity);
		}
		const bool any_entity_took_damage = !_entities_that_took_damage.empty();
		_entities_that_took_damage.clear();
		return any_entity_took_damage;
	}

	bool deal_damage_in_circle(const DamageEvent& ev, const Vec2f& center, float radius, uint32_t mask_bits) {
		//debug::draw_circle(center, radius, Color::Red, 0.2f);
		for (const OverlapHit& hit : overlap_circle(center, radius, mask_bits)) {
			if (hit.entity == ev.source) continue; // For now, entities can't damage themselves
			if (_entities_that_took_damage.contains(hit.entity)) continue;
			if (!deal_damage(hit.entity, ev)) continue;
			_entities_that_took_damage.insert(hit.entity);
		}
		const bool any_entity_took_damage = !_entities_that_took_damage.empty();
		_entities_that_took_damage.clear();
		return any_entity_took_damage;
	}
}