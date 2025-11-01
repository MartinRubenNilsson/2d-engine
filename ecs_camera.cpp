#include "stdafx.h"
#include "ecs_camera.h"
#include "ecs_sprites.h"
#include "ecs_tiled.h"
#include "ecs_tags.h"
#include "random.h"
#include "easings.h"

namespace ecs {
	extern entt::registry _registry;

	void setup_cameras(MapId map) {
		const Vec2f map_size_in_pixels = get_size_in_pixels(map);

		for (auto [entity, object] : _registry.view<Type<Tag::Camera>, ObjectId>().each()) {
			Camera& camera = _registry.emplace<Camera>(entity);
			camera.center = get_center(object);
			camera.bounds.min = Vec2f::ZERO;
			camera.bounds.max = map_size_in_pixels;
			camera.entity_to_follow = get_entity(object, "follow");
		}
	}

	// Returns the new center of the camera.
	Vec2f _confine_camera(const Vec2f& center, const Vec2f& size, const Rect2f& bounds) {
		const Vec2f center_min = bounds.min + size * 0.5f;
		const Vec2f center_max = bounds.max - size * 0.5f;
		Vec2f new_center = center;
		if (center_min.x < center_max.x) {
			new_center.x = std::clamp(new_center.x, center_min.x, center_max.x);
		} else {
			new_center.x = (bounds.min.x + bounds.max.x) / 2.f;
		}
		if (center_min.y < center_max.y) {
			new_center.y = std::clamp(new_center.y, center_min.y, center_max.y);
		} else {
			new_center.y = (bounds.min.y + bounds.max.y) / 2.f;
		}
		return new_center;
	}

	constexpr float _CAMERA_BLEND_DURATION = 1.f;

	float _camera_shake_time = 0.f;
	float _camera_blend_time = _CAMERA_BLEND_DURATION;

	void update_cameras(float dt) {
		_camera_shake_time += dt;
		_camera_blend_time = std::clamp(_camera_blend_time + dt, 0.f, _CAMERA_BLEND_DURATION);

		for (auto [entity, camera] : _registry.view<Camera>().each()) {

			// If the camera has a follow target, center the camera on the target's sprite's center.
			//
			// PITFALL: I first tried centering the camera on the target's body's center of mass, but
			// this lead to issues with pixel snapping. Both the sprite and the camera snap to pixels,
			// but since the body center isn't necessarily an integer number of pixels away from the
			// sprite center, this caused the snapping to sometimes happen in different directions.
			// As a result the sprite being followed by the camera sometimes appeared to jitter.
			//
			if (camera.entity_to_follow != entt::null) {
				if (const sprites::Sprite* sprite = _registry.try_get<sprites::Sprite>(camera.entity_to_follow)) {
					// PITFALL: When rendering the sprites we round their position in order to snap to pixels.
					const Vec2f sprite_render_position = round(sprite->position);
					const Vec2f sprite_render_center = sprite_render_position + round(sprite->size * 0.5f);
					camera.center = sprite_render_center;
				}
			}

			camera.center = _confine_camera(camera.center, camera.size, camera.bounds);
			camera.trauma = std::clamp(camera.trauma - camera.trauma_decay * dt, 0.f, 1.f);

			camera._shake_offset = Vec2f::ZERO;
			if (camera.shake_amplitude && camera.shake_frequency && camera.trauma) {
				float total_shake_amplitude = camera.shake_amplitude * camera.trauma * camera.trauma;
				camera._shake_offset.x = total_shake_amplitude *
					random::fractal_perlin_noise(0, camera.shake_frequency * _camera_shake_time);
				camera._shake_offset.y = total_shake_amplitude *
					random::fractal_perlin_noise(1, camera.shake_frequency * _camera_shake_time);
			}

			// Make sure shake_offset doesn't push the camera outside its confines.
			Vec2f shaken_center = camera.center + camera._shake_offset;
			shaken_center = _confine_camera(shaken_center, camera.size, camera.bounds);
			camera._shake_offset = shaken_center - camera.center;

			camera.center = round(camera.center); // Snap to pixels
		}
	}

	Rect2f get_default_camera_view() {
		return { .min = Vec2f::ZERO, .max = DEFAULT_CAMERA_SIZE };
	}

	Rect2f get_camera_view(entt::entity entity) {
		const Camera* camera = get_camera(entity);
		if (!camera) return get_default_camera_view();
		Rect2f view{};
		view.min = camera->center - camera->size * 0.5f;
		view.max = camera->center + camera->size * 0.5f;
		view.min += camera->_shake_offset;
		view.max += camera->_shake_offset;
		return view;
	}

	entt::entity _prev_active_camera_entity = entt::null;
	entt::entity _curr_active_camera_entity = entt::null;

	Rect2f get_active_camera_view() {
		return get_camera_view(_curr_active_camera_entity);
	}

	Rect2f get_blended_camera_view() {
		const Rect2f prev_view = get_camera_view(_prev_active_camera_entity);
		const Rect2f curr_view = get_camera_view(_curr_active_camera_entity);
		const float blend_factor = ease_out_expo(_camera_blend_time / _CAMERA_BLEND_DURATION);
		return lerp(prev_view, curr_view, blend_factor);
	}

	bool activate_camera(entt::entity entity, bool hard_cut) {
		if (!_registry.all_of<Camera>(entity))
			return false;
		_prev_active_camera_entity = _curr_active_camera_entity;
		_curr_active_camera_entity = entity;
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
		if (_curr_active_camera_entity == entity) {
			_curr_active_camera_entity = new_entity;
		}
		return new_entity;
	}

	void add_trauma_to_camera(entt::entity entity, float trauma) {
		if (Camera* camera = get_camera(entity)) {
			camera->trauma += trauma;
		}
	}

	void add_trauma_to_active_camera(float trauma) {
		if (_curr_active_camera_entity == entt::null) return;
		add_trauma_to_camera(_curr_active_camera_entity, trauma);
	}
}