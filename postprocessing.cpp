#include "stdafx.h"
#include "postprocessing.h"
#include "graphics.h"
#include "graphics_globals.h"

namespace postprocessing {
	struct Shockwave {
		Vec2f position_ws; // ws = world space
		float force = 0.f;
		float size = 0.f;
		float thickness = 0.f;
	};

	const size_t MAX_GAUSSIAN_BLUR_ITERATIONS = 5;

	std::vector<Shockwave> _shockwaves;
	float _darkness_intensity = 0.f;
	Vec2f _darkness_center_ws; // ws = world space
	float _screen_transition_progress = 0.f;
	size_t _gaussian_blur_iterations = 0;

	void _update_shockwaves(float dt) {
		//TODO: this probably belongs in the ecs
		for (auto it = _shockwaves.begin(); it != _shockwaves.end();) {
			it->force -= dt * 0.4f;
			it->size += dt * 0.7f;
			it->thickness += dt * 0.2f;
			if (it->force <= 0.f) {
				it = _shockwaves.erase(it);
			} else {
				++it;
			}
		}
	}

	void update(float dt) {
		_update_shockwaves(dt);
	}

	Vec2f _map_world_to_target(
		const Vec2f& pos_ws,
		const Vec2f& camera_min_ws,
		const Vec2f& camera_max_ws,
		unsigned int target_width,
		unsigned int target_height
	) {
		const float x = (pos_ws.x - camera_min_ws.x) / (camera_max_ws.x - camera_min_ws.x) * target_width;
		const float y = (pos_ws.y - camera_min_ws.y) / (camera_max_ws.y - camera_min_ws.y) * target_height;
		return Vec2f(x, y);
	}

	void _render_shockwaves(const Vec2f& camera_min, const Vec2f& camera_max) {
		if (_shockwaves.empty()) return;
		if (graphics::shockwave_frag == Handle<graphics::FragmentShader>()) return;
		graphics::ScopedDebugGroup debug_group("postprocessing::_render_shockwaves()");
		graphics::bind_fragment_shader(graphics::shockwave_frag);
		graphics::bind_uniform_buffer(1, graphics::shockwave_uniform_buffer);
		graphics::ShockwaveUniformBlock shockwave_ub{};
		shockwave_ub.resolution = Vec2f(GAME_FRAMEBUFFER_WIDTH, GAME_FRAMEBUFFER_HEIGHT);
		for (const Shockwave& shockwave : _shockwaves) {
			shockwave_ub.center = _map_world_to_target(
				shockwave.position_ws, camera_min, camera_max,
				GAME_FRAMEBUFFER_WIDTH, GAME_FRAMEBUFFER_HEIGHT);
			shockwave_ub.force = shockwave.force;
			shockwave_ub.size = shockwave.size;
			shockwave_ub.thickness = shockwave.thickness;
			graphics::update_buffer(graphics::shockwave_uniform_buffer, &shockwave_ub, sizeof(shockwave_ub));
			std::swap(graphics::game_ping_framebuffer, graphics::game_pong_framebuffer);
			graphics::bind_framebuffer(graphics::game_ping_framebuffer);
			graphics::bind_texture(0, graphics::get_framebuffer_texture(graphics::game_pong_framebuffer));
			graphics::draw(3); // draw a fullscreen-covering triangle
		}
	}

	void _render_darkness(const Vec2f& camera_min, const Vec2f& camera_max) {
		if (_darkness_intensity == 0.f) return;
		if (graphics::darkness_frag == Handle<graphics::FragmentShader>()) return;
		graphics::ScopedDebugGroup debug_group("postprocesssing::_render_darkness()");
		std::swap(graphics::game_ping_framebuffer, graphics::game_pong_framebuffer);
		graphics::bind_framebuffer(graphics::game_ping_framebuffer);
		graphics::bind_fragment_shader(graphics::darkness_frag);
		graphics::DarknessUniformBlock darkness_ub{};
		darkness_ub.resolution = Vec2f(GAME_FRAMEBUFFER_WIDTH, GAME_FRAMEBUFFER_HEIGHT);
		darkness_ub.center = _map_world_to_target(
			_darkness_center_ws, camera_min, camera_max,
			GAME_FRAMEBUFFER_WIDTH, GAME_FRAMEBUFFER_HEIGHT);
		darkness_ub.intensity = _darkness_intensity;
		graphics::update_buffer(graphics::darkness_uniform_buffer, &darkness_ub, sizeof(darkness_ub));
		graphics::bind_uniform_buffer(1, graphics::darkness_uniform_buffer);
		graphics::bind_texture(0, graphics::get_framebuffer_texture(graphics::game_pong_framebuffer));
		graphics::draw(3); // draw a fullscreen-covering triangle
	}

	void _render_screen_transition() {
		if (_screen_transition_progress == 0.f) return;
		if (graphics::screen_transition_frag == Handle<graphics::FragmentShader>()) return;
		graphics::ScopedDebugGroup debug_group("postprocessing::_render_screen_transition()");
		graphics::bind_fragment_shader(graphics::screen_transition_frag);
		graphics::ScreenTransitionUniformBlock screen_transition_ub{};
		screen_transition_ub.progress = _screen_transition_progress;
		graphics::update_buffer(graphics::screen_transition_uniform_buffer, &screen_transition_ub, sizeof(screen_transition_ub));
		graphics::bind_uniform_buffer(1, graphics::screen_transition_uniform_buffer);
		std::swap(graphics::game_ping_framebuffer, graphics::game_pong_framebuffer);
		graphics::bind_framebuffer(Handle<graphics::Framebuffer>()); // ensure pong framebuffer is unbound
		graphics::bind_texture(0, graphics::get_framebuffer_texture(graphics::game_pong_framebuffer));
		graphics::bind_framebuffer(graphics::game_ping_framebuffer);
		graphics::draw(3); // draw a fullscreen-covering triangle
	}

	void _render_gaussian_blur() {
		if (_gaussian_blur_iterations == 0) return;
		if (graphics::gaussian_blur_hor_frag == Handle<graphics::FragmentShader>()) return;
		if (graphics::gaussian_blur_ver_frag == Handle<graphics::FragmentShader>()) return;
		graphics::ScopedDebugGroup debug_group("postprocessing::_render_gaussian_blur()");
		graphics::bind_sampler(0, graphics::linear_sampler);
		for (size_t i = 0; i < _gaussian_blur_iterations; ++i) {
			// Horizontal pass
			graphics::bind_fragment_shader(graphics::gaussian_blur_hor_frag);
			std::swap(graphics::game_ping_framebuffer, graphics::game_pong_framebuffer);
			graphics::bind_framebuffer(Handle<graphics::Framebuffer>()); // ensure pong framebuffer is unbound
			graphics::bind_texture(0, graphics::get_framebuffer_texture(graphics::game_pong_framebuffer));
			graphics::bind_framebuffer(graphics::game_ping_framebuffer);
			graphics::draw(3); // draw a fullscreen-covering triangle
			// Vertical pass
			graphics::bind_fragment_shader(graphics::gaussian_blur_ver_frag);
			std::swap(graphics::game_ping_framebuffer, graphics::game_pong_framebuffer);
			graphics::bind_framebuffer(Handle<graphics::Framebuffer>()); // ensure pong framebuffer is unbound
			graphics::bind_texture(0, graphics::get_framebuffer_texture(graphics::game_pong_framebuffer));
			graphics::bind_framebuffer(graphics::game_ping_framebuffer);
			graphics::draw(3); // draw a fullscreen-covering triangle
		}
		graphics::bind_sampler(0, graphics::nearest_sampler);
	}

	void render(const Vec2f& camera_min, const Vec2f& camera_max) {
		graphics::ScopedDebugGroup debug_group("postprocessing::render()");
		graphics::set_primitives(graphics::Primitives::TriangleList);
		graphics::bind_vertex_shader(graphics::fullscreen_vert);
		_render_shockwaves(camera_min, camera_max);
		_render_darkness(camera_min, camera_max);
		_render_screen_transition();
		_render_gaussian_blur();
	}

	void add_shockwave(const Vec2f& position_ws) {
		Shockwave shockwave{};
		shockwave.position_ws = position_ws; // ws = world space
		shockwave.force = 0.2f;
		shockwave.size = 0.f;
		shockwave.thickness = 0.f;
		_shockwaves.push_back(shockwave);
	}

	void set_darkness_intensity(float intensity) {
		_darkness_intensity = std::clamp(intensity, 0.f, 1.f);
	}

	void set_darkness_center(const Vec2f& position_in_world_space) {
		_darkness_center_ws = position_in_world_space;
	}

	void set_screen_transition_progress(float progress) {
		_screen_transition_progress = std::clamp(progress, -1.f, 1.f);
	}

	void set_gaussian_blur_iterations(size_t iterations) {
		_gaussian_blur_iterations = std::min(iterations, MAX_GAUSSIAN_BLUR_ITERATIONS);
	}
}
