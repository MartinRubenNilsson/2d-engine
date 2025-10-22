#pragma once

namespace window {
	struct Settings {
		static constexpr int MIN_SCALE = 3;
		static constexpr int MAX_SCALE = 7;

		bool fullscreen = false;
		int scale = 5; // Size relative to GAME_FRAMEBUFFER_WIDTH/HEIGHT.
		//bool vsync = false; // Needs to be stored in graphics_settings.h <- TODO
	};

	void get_settings(Settings& settings);
	void apply_settings(const Settings& settings);

	void to_json(rapidjson::Document& doc, const Settings& settings);
	void from_json(Settings& settings, const rapidjson::Document& doc);
}