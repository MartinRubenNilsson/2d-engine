#pragma once

namespace ecs {
	enum class DamageType {
		Default,
		Touch,
		Projectile,
		Explosion,
	};

	struct Damage {
		DamageType type = DamageType::Default;
		int amount = 0;
		// The entity that dealt the damage (e.g. the bomb that exploded).
		// As a safety measure, the source entity cannot damage itself.
		entt::entity source = entt::null;
	};

	// Called when the entity is meant to take damage. Should return true if any damage was applied.
	using DamageHandler = bool(*)(entt::entity entity, const Damage& damage);

	void set_damage_handler(entt::entity entity, DamageHandler handler);
	DamageHandler get_damage_handler(entt::entity entity); // Returns nullptr if handler isn't set

	// Appplies damage to a single entity by calling the entity's DamageHandler.
	bool apply_damage(entt::entity entity, const Damage& damage);

	// Applies damage to all entities that intersect the given box. Returns true if any entity was damaged.
	bool apply_damage_in_box(const Damage& damage, const Vec2f& box_min, const Vec2f& box_max, uint32_t mask_bits = UINT32_MAX);
	// Applies damage to all entities that intersect the given circle. Returns true if any entity was damaged.
	bool apply_damage_in_circle(const Damage& damage, const Vec2f& center, float radius, uint32_t mask_bits = UINT32_MAX);
}
