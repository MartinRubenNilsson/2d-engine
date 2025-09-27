#include "stdafx.h"
#include "ecs_blade_trap.h"
#include "ecs_sprites.h"
#include "ecs_physics.h"
#include "ecs_physics_filters.h"
#include "ecs_damage.h"
#include "ecs_state_machine.h"
#include "audio.h"

namespace ecs {

	constexpr float _BLADE_TRAP_EXTEND_SPEED = 16.f * 6.f; // 6 tiles per second
	constexpr float _BLADE_TRAP_RETRACT_SPEED = 16.f * 2.f; // 2 tiles per second

	extern entt::registry _registry;

	Vector2f _get_direction_for_update_count(unsigned int update_count) {
		switch (update_count % 4) {
			case 0: return { 1.f, 0.f };
			case 1: return { 0.f, 1.f };
			case 2: return { -1.f, 0.f };
			case 3: return { 0.f, -1.f };
		}
		return { 0.f, 0.f }; // should never happen
	}

	void _update_idle_blade_trap(entt::entity entity, float dt)  {
		BladeTrap& trap = _registry.get<BladeTrap>(entity);
		b2BodyId& body = _registry.get<b2BodyId>(entity);
		// Raycast in a cardinal direction to see if there's moving in it.
		trap.direction = _get_direction_for_update_count(trap.update_count);
		const Vector2f ray_start = b2Body_GetPosition(body);
		const Vector2f ray_end = ray_start + trap.direction * 16.f * 10.f; // raycast 10 tiles
		RaycastHit hit{};
		if (!raycast_closest(ray_start, ray_end, CM_Default, &hit))
			return; // We don't see anything.
		if (is_zero(b2Body_GetLinearVelocity(hit.body)))
			return; // We see something, but it's not moving.
		// Start extending.
		transition_state_machine(entity, "extending");
	}

	void _start_extending_blade_trap(entt::entity entity) {
		BladeTrap& trap = _registry.get<BladeTrap>(entity);
		b2BodyId& body = _registry.get<b2BodyId>(entity);
		// Start moving in direction.
		b2Body_SetType(body, b2_dynamicBody);
		b2Body_SetLinearVelocity(body, trap.direction * _BLADE_TRAP_EXTEND_SPEED);
		// Start playing extension sound.
		trap.audio_event = audio::create_event({ .path = "event:/blade_trap/extend" });
	}

	void _stop_extending_blade_trap(entt::entity entity) {
		BladeTrap& trap = _registry.get<BladeTrap>(entity);
		// Stop playing extension sound.
		audio::stop_event(trap.audio_event);
	}

	void _start_impacting_blade_trap(entt::entity entity) {
		b2BodyId& body = _registry.get<b2BodyId>(entity);
		// Shake a lot.
		emplace_sprite_shake(entity, {
			.duration = 0.4f,
			.magnitude = 7.f,
			.exponent = 3.f });
		// Play impact sound.
		audio::create_event({
			.path = "event:/blade_trap/impact",
			.position = b2Body_GetPosition(body) });
		// Start retracting in 0.4 seconds.
		transition_state_machine_later(entity, "retracting", 0.4f);
	}

	void _start_retracting_blade_trap(entt::entity entity) {
		BladeTrap& trap = _registry.get<BladeTrap>(entity);
		// Start playing retraction sound.
		trap.audio_event = audio::create_event({ .path = "event:/blade_trap/retract" });
	}

	void _update_retracting_blade_trap(entt::entity entity, float dt) {
		BladeTrap& trap = _registry.get<BladeTrap>(entity);
		b2BodyId& body = _registry.get<b2BodyId>(entity);
		// Check if we're within one pixel of the start position.
		const Vector2f to_start = trap.start_position - b2Body_GetPosition(body);
		const float dist_to_start = length(to_start);
		if (dist_to_start >= 1.f) {
			const Vector2 dir_to_start = to_start / dist_to_start;
			b2Body_SetLinearVelocity(body, dir_to_start * _BLADE_TRAP_RETRACT_SPEED);
			return;
		}
		// Go back to being idle, but wait 0.2 seconds first.
		transition_state_machine(entity, "wait");
		transition_state_machine_later(entity, "idle", 0.2f);
	}

	void _stop_retracting_blade_trap(entt::entity entity) {
		BladeTrap& trap = _registry.get<BladeTrap>(entity);
		b2BodyId& body = _registry.get<b2BodyId>(entity);
		// Stop moving.
		b2Body_SetType(body, b2_staticBody);
		b2Body_SetTransform(body, trap.start_position, b2Rot_identity);
		// Shake a little.
		emplace_sprite_shake(entity, {
			.duration = 0.2f,
			.magnitude = 6.f,
			.exponent = 2.f });
		// Stop playing retraction sound.
		audio::stop_event(trap.audio_event);
		// Play reset sound.
		audio::create_event({
			.path = "event:/blade_trap/reset",
			.position = b2Body_GetPosition(body) });
	}

	void _emplace_blade_trap_state_machine(entt::entity entity) {
		StateMachine& sm = emplace_state_machine(entity);
		add_state(sm, {
			.id = "idle",
			.update = _update_idle_blade_trap });
		add_state(sm, {
			.id = "extending",
			.enter = _start_extending_blade_trap,
			.exit = _stop_extending_blade_trap });
		add_state(sm, {
			.id = "impacting",
			.enter = _start_impacting_blade_trap });
		add_state(sm, {
			.id = "retracting",
			.exit = _stop_retracting_blade_trap, 
			.update = _update_retracting_blade_trap });
		add_state(sm, {
			.id = "wait" });
		transition(sm, "idle", entity);
	}

	void update_blade_traps(float dt) {
		for (auto [entity, blade_trap, body] : _registry.view<BladeTrap, b2BodyId>().each()) {
			blade_trap.update_count++;
			// Update audio position.
			if (blade_trap.audio_event != Handle<audio::Event>()) {
				audio::set_event_position(blade_trap.audio_event, b2Body_GetPosition(body));
			}
		}
	}

	BladeTrap& emplace_blade_trap(entt::entity entity) {
		_emplace_blade_trap_state_machine(entity); // HACK
		return _registry.emplace_or_replace<BladeTrap>(entity);
	}

	BladeTrap* get_blade_trap(entt::entity entity) {
		return _registry.try_get<BladeTrap>(entity);
	}

	void on_blade_trap_physics_event(const PhysicsEvent& ev) {
		if (ev.type == PhysicsEventType::ContactBeginTouch) {
			transition_state_machine(ev.entity_a, "impacting");
			apply_damage(ev.entity_b, { .type = DamageType::Melee, .amount = 1 });
		}
	}
}