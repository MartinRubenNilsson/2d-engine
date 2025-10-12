#pragma once

namespace ecs {
	enum class DamageType {
		Default,
		Touch,
		Melee,
		Projectile,
		Explosion,
	};

	struct DamageEvent {
		DamageType type = DamageType::Default;
		int amount = 0;
		// The entity that dealt the damage (e.g. the bomb that exploded).
		// As a safety measure, the source entity cannot damage itself.
		entt::entity source = entt::null;
	};

	// Called when the entity is meant to take damage. Should return true if any damage was applied.
	using DamageEventHandler = bool(*)(entt::entity entity, const DamageEvent& ev);

	void set_damage_event_handler(entt::entity entity, DamageEventHandler handler);
	DamageEventHandler get_damage_event_handler(entt::entity entity); // Returns nullptr if handler isn't set

	// Deals damage to a single entity by calling the entity's DamageHandler.
	bool deal_damage(entt::entity entity, const DamageEvent& ev);

	// Deals damage to all entities that intersect the given box. Returns true if any entity was damaged.
	bool deal_damage_in_box(const DamageEvent& ev, const Vec2f& box_min, const Vec2f& box_max, uint32_t mask_bits = UINT32_MAX);
	// Deals damage to all entities that intersect the given circle. Returns true if any entity was damaged.
	bool deal_damage_in_circle(const DamageEvent& ev, const Vec2f& center, float radius, uint32_t mask_bits = UINT32_MAX);
}
