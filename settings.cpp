#include "stdafx.h"
#include "settings.h"
#include "window.h"
#include "graphics.h"
#include "audio.h"
#include "files.h"

namespace settings {

	AppSettings app_settings;

	void apply(const AppSettings& settings) {
		window::set_fullscreen(settings.fullscreen);
		if (!settings.fullscreen) {
			window::set_size({
				GAME_FRAMEBUFFER_WIDTH * (int)settings.window_scale,
				GAME_FRAMEBUFFER_HEIGHT * (int)settings.window_scale });
		}
		graphics::set_swap_chain_sync_interval(settings.vsync ? 1 : 0);
		audio::set_bus_volume(audio::BUS_MASTER, settings.volume_master);
		audio::set_bus_volume(audio::BUS_MUSIC, settings.volume_music);
		audio::set_bus_volume(audio::BUS_SOUND, settings.volume_sound);
	}

	void save_to_stream(std::ostream& os, const AppSettings& settings) {
		os << "fullscreen " << settings.fullscreen << std::endl;
		os << "window_scale " << settings.window_scale << std::endl;
		os << "vsync " << settings.vsync << std::endl;
		os << "volume_master " << settings.volume_master << std::endl;
		os << "volume_music " << settings.volume_music << std::endl;
		os << "volume_sound " << settings.volume_sound << std::endl;
	}

	void load_from_stream(std::istream& is, AppSettings& settings) {
		std::string line;
		while (std::getline(is, line)) {
			std::istringstream iss(std::move(line));
			std::string key;
			iss >> key;
			if (key == "fullscreen") {
				iss >> settings.fullscreen;
			} else if (key == "window_scale") {
				iss >> settings.window_scale;
			} else if (key == "vsync") {
				iss >> settings.vsync;
			} else if (key == "volume_master") {
				iss >> settings.volume_master;
			} else if (key == "volume_music") {
				iss >> settings.volume_music;
			} else if (key == "volume_sound") {
				iss >> settings.volume_sound;
			}
		}
	}

	const std::string_view APP_SETTINGS_PATH = "settings.txt";

	bool save_to_file(std::string_view path, const AppSettings& settings) {
		std::ostringstream oss;
		save_to_stream(oss, settings);
		return files::write_text_file(path, oss.str());
	}

	bool load_from_file(std::string_view path, AppSettings& settings) {
		std::string text;
		if (!files::read_text_file(path, text)) return false;
		std::istringstream iss(std::move(text));
		load_from_stream(iss, settings);
		return true;
	}
}
