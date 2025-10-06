#include "stdafx.h"
#include "ecs_camera.h"
#include "ecs_physics.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "random.h"
#include "easings.h"

namespace ecs {
	const Vec2f DEFAULT_CAMERA_SIZE = { GAME_FRAMEBUFFER_WIDTH, GAME_FRAMEBUFFER_HEIGHT };
	const float _CAMERA_BLEND_DURATION = 1.f;

	extern entt::registry _registry;

	entt::entity _active_camera_entity = entt::null;
	Vec2f _last_active_camera_center;
	Vec2f _last_active_camera_size = DEFAULT_CAMERA_SIZE;
	Vec2f _active_camera_center;
	Vec2f _active_camera_size = DEFAULT_CAMERA_SIZE;
	float _camera_shake_time = 0.f;
	float _camera_blend_time = _CAMERA_BLEND_DURATION;

	Vec2f _confine_camera_center(
		const Vec2f& camera_center,
		const Vec2f& camera_size,
		const Vec2f& confines_min,
		const Vec2f& confines_max) {
		const Vec2f center_min = confines_min + camera_size / 2.f;
		const Vec2f center_max = confines_max - camera_size / 2.f;
		Vec2f confined_center = camera_center;
		if (center_min.x < center_max.x) {
			confined_center.x = std::clamp(confined_center.x, center_min.x, center_max.x);
		} else {
			confined_center.x = (confines_min.x + confines_max.x) / 2.f;
		}
		if (center_min.y < center_max.y) {
			confined_center.y = std::clamp(confined_center.y, center_min.y, center_max.y);
		} else {
			confined_center.y = (confines_min.y + confines_max.y) / 2.f;
		}
		return confined_center;
	}

	void setup_cameras(MapId map) {
		const Vec2f map_bottom_right = get_size_in_pixels(map);

		for (auto [entity, object] : _registry.view<Type<Tag::Camera>, ObjectId>().each()) {
			Camera& camera = _registry.emplace<Camera>(entity);
			camera.center = get_position(object);
			camera.confines_min = { 0.f, 0.f };
			camera.confines_max = map_bottom_right;
			camera.entity_to_follow = get_entity(object, "follow");
		}
	}

	void update_cameras(float dt) {
		_camera_shake_time += dt;
		_camera_blend_time = std::clamp(_camera_blend_time + dt, 0.f, _CAMERA_BLEND_DURATION);

		for (auto [entity, camera] : _registry.view<Camera>().each()) {

			// If the camera has a follow target, center the view on the target.
			if (b2BodyId body = get_body(camera.entity_to_follow); B2_IS_NON_NULL(body)) {
				camera.center = b2Body_GetWorldCenterOfMass(body);
			}

			camera.center = _confine_camera_center(camera.center, camera.size, camera.confines_min, camera.confines_max);
			camera.trauma = std::clamp(camera.trauma - camera.trauma_decay * dt, 0.f, 1.f);

			camera.shake_offset = { 0.f, 0.f };
			if (camera.shake_amplitude && camera.shake_frequency && camera.trauma) {
				float total_shake_amplitude = camera.shake_amplitude * camera.trauma * camera.trauma;
				camera.shake_offset.x = total_shake_amplitude *
					random::fractal_perlin_noise(0, camera.shake_frequency * _camera_shake_time);
				camera.shake_offset.y = total_shake_amplitude *
					random::fractal_perlin_noise(1, camera.shake_frequency * _camera_shake_time);
			}

			// Make sure shake_offset doesn't push the camera outside its confines.
			Vec2f shaky_center = camera.center + camera.shake_offset;
			shaky_center = _confine_camera_center(shaky_center, camera.size, camera.confines_min, camera.confines_max);
			camera.shake_offset = shaky_center - camera.center;
		}

		// Update the active camera view.
		if (Camera* camera = get_camera(_active_camera_entity)) {
			_active_camera_center = camera->center + camera->shake_offset;
			_active_camera_size = camera->size;
		} else {
			_active_camera_center = { 0.f, 0.f };
			_active_camera_size = DEFAULT_CAMERA_SIZE;
		}
	}

	void get_active_camera_view(Vec2f& center, Vec2f& size) {
		center = _active_camera_center;
		size = _active_camera_size;
	}

	void get_blended_camera_view(Vec2f& center, Vec2f& size) {
		float blend_factor = ease_out_expo(_camera_blend_time / _CAMERA_BLEND_DURATION);
		center = lerp(_last_active_camera_center, _active_camera_center, blend_factor);
		size = lerp(_last_active_camera_size, _active_camera_size, blend_factor);
	}

	bool activate_camera(entt::entity entity, bool hard_cut) {
		if (!_registry.all_of<Camera>(entity)) return false;
		_last_active_camera_center = _active_camera_center;
		_last_active_camera_size = _active_camera_size;
		_active_camera_entity = entity;
		_camera_blend_time = hard_cut ? _CAMERA_BLEND_DURATION : 0.f;
		return true;
	}

	Camera& emplace_camera(entt::entity entity, const Camera& camera) {
		return _registry.emplace_or_replace<Camera>(entity, camera);
	}

	Camera* get_camera(entt::entity entity) {
		return _registry.try_get<Camera>(entity);
	}

	entt::entity detach_camera(entt::entity entity) {
		Camera* camera = get_camera(entity);
		if (!camera) return entt::null;
		camera->entity_to_follow = entt::null;
		entt::entity new_entity = _registry.create();
		_registry.emplace<Camera>(new_entity, *camera);
		_registry.remove<Camera>(entity);
		if (_active_camera_entity == entity) {
			_active_camera_entity = new_entity;
		}
		return new_entity;
	}

	bool add_trauma_to_camera(entt::entity entity, float trauma) {
		if (Camera* camera = get_camera(entity)) {
			camera->trauma += trauma;
			return true;
		}
		return false;
	}

	bool add_trauma_to_active_camera(float trauma) {
		if (_active_camera_entity == entt::null) return false;
		return add_trauma_to_camera(_active_camera_entity, trauma);
	}
}