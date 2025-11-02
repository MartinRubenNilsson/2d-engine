#include "stdafx.h"
#include "engine_fps_counter.h"
#include "steam.h"
#include "files.h"
#include "networking.h"
#include "window.h"
#include "window_events.h"
#include "audio.h"
#include "ui.h"
#include "ui_game.h"
#include "map.h"
#include "ecs.h"
#include "console.h"
#include "background.h"
#include "postprocessing.h"
#include "settings.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "renderdoc.h"
#include "imgui_impl.h"
#include "input.h"
#include "text_fonts.h"
#include "platform.h"
#include "platform_directory_changes.h"
#include "editor.h"

namespace engine {
    bool _should_run = false;

    void startup(int argc, char* argv[]) {
        setlocale(LC_ALL, "en_US.utf8");
        if (steam::restart_app_if_necessary())
            return;
        steam::startup(); // Fails silently if Steam is not running.
        files::startup();
        networking::startup();
        renderdoc::startup();
        window::startup();
        graphics::startup();
        graphics::startup_globals();
        imgui_impl::startup();
        console::startup();
        postprocessing::startup();
        audio::startup();
        text::startup_fonts();
        ui::startup();
        ui::game::startup();
        ecs::startup();
        settings::load_from_file(settings::APP_SETTINGS_PATH, settings::app_settings);
        settings::apply(settings::app_settings);
        //window::set_position(Vec2i::ZERO);
        window::set_visible(true);
        console::execute(argc, argv);
        _should_run = true;
    }

    void shutdown() {
        ecs::shutdown();
        ui::game::shutdown();
        ui::shutdown();
        text::shutdown_fonts();
        audio::shutdown();
        imgui_impl::shutdown();
        graphics::shutdown();
        window::shutdown();
        networking::shutdown();
        steam::shutdown();
        platform::shutdown_directory_watchers();
    }

    bool should_run() {
        return _should_run;
    }

    double _time = 0.0; // total time since engine startup
    double _delta_time = 0.0; // time since last call to run()
    double _game_time = 0.0;

    void _update_times() {
        const double prev_time = _time;
        _time = window::get_elapsed_time();
        if (prev_time == 0.0) return; // Avoid big delta time right after startup().
        _delta_time = _time - prev_time;
    }

    void _update() {
        _update_times();
        engine::update_fps_counter((float)_delta_time);
        platform::update_directory_watchers();
        steam::run_message_loop();
        window::update_events();
        graphics::handle_window_events();
        graphics::handle_window_events_for_globals();
        imgui_impl::new_frame();
        console::handle_window_events();
        input::handle_window_events();

        if (window::should_close()) {
            _should_run = false;
        }

        audio::update();
        console::update((float)_delta_time);

        if (map::has_current_map()) {
            background::set_type(background::Type::None);
        } else {
            background::set_type(background::Type::MountainDusk);
            background::update((float)_delta_time); // TODO: this doesn't belong in engine.cpp
        }

        map::update((float)_delta_time); // TODO: this doesn't belong in engine.cpp

        bool should_update_game = true;
        if (steam::is_overlay_active()) {
            should_update_game = false;
        }
        if (ui::game::should_pause_game()) { // TODO: this doesn't belong in engine.cpp
            should_update_game = false;
        }
        if (map::get_transition_progress() != 0.f) {
            should_update_game = false; // pause game while map is transitioning
        }

        if (should_update_game) {
            const double game_delta_time = _delta_time;
            _game_time += game_delta_time;
            ecs::update((float)game_delta_time);
            postprocessing::update((float)game_delta_time);
        }

        ui::update((float)_delta_time);
        ui::game::update((float)_delta_time);
    }

    void _render(); // engine_rendering.cpp

    void run() {
        _update();
        _render();
        renderdoc::open_capture_directory_if_frame_capturing();
    }
}