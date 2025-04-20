#include "stdafx.h"
#include "ecs_damage.h"
#include "ecs_common.h"
#include "ecs_physics.h"
#include "ecs_pickups.h"
#include "random.h"
#include "audio.h"

namespace ecs {

	extern entt::registry _registry;

	void set_apply_damage_callback(entt::entity entity, ApplyDamageCallback callback) {
		_registry.emplace_or_replace<ApplyDamageCallback>(entity, callback);
	}

	ApplyDamageCallback get_apply_damage_callback(entt::entity entity) {
		ApplyDamageCallback* callback_ptr = _registry.try_get<ApplyDamageCallback>(entity);
		return callback_ptr ? *callback_ptr : nullptr;
	}

	bool apply_damage(entt::entity entity, const Damage& damage) {
		if (entity == entt::null) return false;
		if (entity == damage.source) return false; // For now, entities can't damage themselves
		ApplyDamageCallback callback = get_apply_damage_callback(entity);
		if (!callback) return false;
		return callback(entity, damage);
	}

	// This is used for record-keeping so we don't apply the same damage to the same entity multiple times.
	std::unordered_set<entt::entity> _entities_that_took_damage;

	bool apply_damage_in_box(const Damage& damage, const Vector2f& box_min, const Vector2f& box_max, uint32_t mask_bits) {
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

	bool apply_damage_in_circle(const Damage& damage, const Vector2f& center, float radius, uint32_t mask_bits) {
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