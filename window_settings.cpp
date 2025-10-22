#include "stdafx.h"
#include "window_settings.h"
#include "window.h"
#include <rapidjson/document.h>

namespace window {
	int _clamp_scale(int scale) {
		return std::clamp(scale, Settings::MIN_SCALE, Settings::MAX_SCALE);
	}

	void get_settings(Settings& settings) {
		settings.fullscreen = window::get_fullscreen();
		settings.scale = window::get_size().x / GAME_FRAMEBUFFER_WIDTH;
		settings.scale = _clamp_scale(settings.scale);
	}

	void apply_settings(const Settings& settings) {
		window::set_fullscreen(settings.fullscreen);
		if (!settings.fullscreen) {
			const int scale = (int)_clamp_scale(settings.scale);
			const Vec2i size = { scale * GAME_FRAMEBUFFER_WIDTH, scale * GAME_FRAMEBUFFER_HEIGHT };
			window::set_size(size);
		}
		//graphics::set_swap_chain_sync_interval(settings.vsync ? 1 : 0);
	}

	void to_json(rapidjson::Document& doc, const Settings& settings) {
		doc["fullscreen"].SetBool(settings.fullscreen);
		doc["scale"].SetUint(settings.scale);
	}

	void from_json(Settings& settings, const rapidjson::Document& doc) {
		settings = {};
		settings.scale = _clamp_scale(settings.scale);
	}
}