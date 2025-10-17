#include "stdafx.h"
#include "steam.h"
#include "files.h"
#include "networking.h"
#include "window.h"
#include "window_events.h"
#include "audio.h"
#include "ui.h"
#include "ui_menus.h"
#include "ui_clay.h"
#include "map.h"
#include "ecs.h"
#include "console.h"
#include "background.h"
#include "postprocessing.h"
#include "settings.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "shapes.h"
#include "sprites.h"
#include "renderdoc.h"
#include "imgui_impl.h"
#include "input.h"
#include "text_fonts.h"
#include "platform.h"
#include "platform_directory_changes.h"

namespace engine {
    bool _should_run = false;
    double _time = 0.f; // since engine startup
    double _delta_time = 0.f; // since last call to run()
    double _game_time = 0.f;
    double _game_delta_time = 0.f;

    bool debug_stats = false;

    void _load_all_audio_banks() {
        for (const files::File& file : files::get_all_files_in_directory("assets/audio/banks")) {
            if (file.format != files::FileFormat::FmodStudioBank) continue;
            audio::load_bank_from_file(file.path);
        }
    }

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
        postprocessing::startup();
        console::startup();
        audio::startup();
        _load_all_audio_banks();
        ui::startup();
        ui::startup_rmlui(); // TODO: remove
        ecs::startup();


        for (const files::File& file : files::get_all_files_in_directory("assets/fonts")) { // TODO: remove
            if (file.format != files::FileFormat::TrueTypeFont) continue;
            ui::load_font_from_file(file.path);
        }
        for (const files::File& file : files::get_all_files_in_directory("assets/ui")) { // TODO: remove
            if (file.format != files::FileFormat::RmlUiDocument) continue;
            ui::load_document_from_file(file.path);
        }

        // TODO: remove
        ui::add_event_listeners(); // Must come after loading RML documents.

        settings::load_from_file(settings::APP_SETTINGS_PATH, settings::app_settings);
        settings::apply(settings::app_settings);

        window::set_visible(true);

        console::execute(argc, argv);

        _should_run = true;
        _time = window::get_elapsed_time();
        
        //platform::start_watching_directory_changes(L"assets", true); // TEST
    }

    void shutdown() {
        ecs::shutdown();
        ui::shutdown_rmlui();
        ui::shutdown();
        text::shutdown_fonts();
        audio::shutdown();
        imgui_impl::shutdown();
        graphics::shutdown();
        window::shutdown();
        networking::shutdown();
        steam::shutdown();
        platform::shutdown();
    }

    bool should_run() {
        return _should_run && !window::should_close();
    }

    void _update_times() {
        const double new_elapsed_time = window::get_elapsed_time();
        _delta_time = new_elapsed_time - _time;
        _time = new_elapsed_time;
    }

    void _update() {
        _update_times();

        platform::update_directory_changes();

        for (const platform::DirectoryChange& change : platform::get_directory_changes()) {
            std::string string{ change.file_path.begin(), change.file_path.end() };
            console::log(string);
        }

        steam::run_message_loop();
        window::update_events();
        imgui_impl::new_frame();
        input::handle_window_events();
        console::handle_window_events();

        // PROCESS WINDOW EVENTS
        {
            for (const window::Event& ev : window::get_events()) {
                if (ev.type == window::EventType::WindowClose) {
                    window::set_should_close(true);
                } else if (ev.type == window::EventType::FramebufferSize) {
                    // When the window is minimized, an event is sent with size 0, 0,
                    // which we must therefore ignore.
                    if (ev.size.width && ev.size.height) {
                        graphics::resize_swap_chain_framebuffer(ev.size.width, ev.size.height);
                        graphics::resize_final_framebuffer(ev.size.width, ev.size.height);
                    }
                } else if (ev.type == window::EventType::KeyPress) {
                    if (ev.key.code == window::Key::F1) {
                        debug_stats = !debug_stats;
                    } else if (ev.key.code == window::Key::F6) {
                        ui::debug = !ui::debug;
                    } else if (ev.key.code == window::Key::F7) {
                        map::debug = !map::debug;
                    }
                }

                ui::handle_window_event_for_rmlui(ev);
            }
        }

        // PROCESS UI EVENTS
        {
            // TODO: move
            ui::Event ev;
            while (ui::get_next_event(ev)) {
                switch (ev.type) {
                    case ui::EventType::PlayGame: {
                        background::set_type(background::Type::None);
                        map::open("summer_forest_00");
                    } break;
                    case ui::EventType::RestartMap: {
                        map::reset();
                    } break;
                    case ui::EventType::GoToMainMenu: {
                        background::set_type(background::Type::MountainDusk);
                        map::close(0.f);
                    } break;
                    case ui::EventType::QuitApp: {
                        window::set_should_close(true);
                    } break;
                }
            }
        }

        // UPDATE

        audio::update();
        console::update(_delta_time);
        background::update(_delta_time); // TODO: this doesn't belong in engine.cpp
        ui::update_rmlui(_delta_time);
        map::update(_delta_time);

        double game_delta_time = _delta_time;
        if (steam::is_overlay_active()) {
            game_delta_time = 0.0;
        }
        if (ui::is_menu_or_textbox_visible()) { // TODO: this doesn't belong in engine.cpp
            game_delta_time = 0.0;
        }
        if (map::get_transition_progress() != 0.f) {
            game_delta_time = 0.0; // pause game while map is transitioning
        }

        _game_time += game_delta_time;

        ecs::update(game_delta_time);
        postprocessing::update(game_delta_time);
    }

    void _show_debug_stats_imgui() {
        static float smoothed_dt = 0.f;
        static float smoothed_fps = 0.f;
        static float dt_buffer[256] = { 0.f };
        static float fps_buffer[256] = { 0.f };
        static int buffer_offset = 0;
        dt_buffer[buffer_offset] = _delta_time;
        fps_buffer[buffer_offset] = 1.f / _delta_time;
        buffer_offset = (buffer_offset + 1) % 256;
        constexpr float SMOOTHING_FACTOR = 0.99f;
        smoothed_dt = SMOOTHING_FACTOR * smoothed_dt + (1.f - SMOOTHING_FACTOR) * _delta_time;
        smoothed_fps = SMOOTHING_FACTOR * smoothed_fps + (1.f - SMOOTHING_FACTOR) / _delta_time;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        char overlay_text[64];
        sprintf(overlay_text, "%.f us", smoothed_dt * 1'000'000.f);
        ImGui::PlotLines("##dt", dt_buffer, 256, buffer_offset, overlay_text, 0.f, 0.01f, ImVec2(0, 80));
        sprintf(overlay_text, "%.f FPS", smoothed_fps);
        ImGui::PlotLines("##fps", fps_buffer, 256, buffer_offset, overlay_text, 0.f, 600.f, ImVec2(0, 80));
        ImGui::End();
    }

    void _render_copy_final_framebuffer_to_back_buffer() {
        const graphics::ScopedDebugGroup debug_group(__FUNCTION__);
        const Handle<graphics::Framebuffer> back_buffer = graphics::get_swap_chain_back_buffer();
        graphics::clear_framebuffer(back_buffer);
        graphics::bind_framebuffer(back_buffer);
#ifdef GRAPHICS_API_OPENGL
        // NOTE: In order to easier handle some differences between OpenGL and D3D11,
        // we render each framebuffer upside-down. This means we can use the same UV
        // and viewport coordinates for both APIs. However, it means we need to do a
        // vertical flip when rendering to the back buffer in OpenGL, otherwise the
        // final image will be upside-down.
        graphics::bind_vertex_shader(graphics::fullscreen_flip_vert);
#else
        graphics::bind_vertex_shader(graphics::fullscreen_vert);
#endif
        graphics::bind_fragment_shader(graphics::fullscreen_frag);
        graphics::bind_texture(0, graphics::get_framebuffer_texture(graphics::final_framebuffer));
        graphics::bind_sampler(0, graphics::nearest_sampler);
        graphics::draw(3); // draw a fullscreen-covering triangle
    }

    void _render() {
        const Vec2i window_framebuffer_size = window::get_framebuffer_size();
        const graphics::Viewport viewport = {
            .width = (float)window_framebuffer_size.x,
            .height = (float)window_framebuffer_size.y };

        Vec2f camera_min = { 0.f, 0.f };
        Vec2f camera_max = window_framebuffer_size;
        if (map::is_open()) {
            ecs::get_camera_bounds(camera_min, camera_max);
        }
        const Vec2f camera_center = (camera_min + camera_max) / 2.f;
        const Vec2f camera_size = camera_max - camera_min;

        // Update frame uniform buffer
        {
            // PITFALL: We use an unusual clip space coordinate system where y is down.
            // This makes it easier to handle some differences between OpenGL and D3D11.
            // Moreover, it means the shader coordinate axes point the same as the game world.
            const float a = 2.f / camera_size.x;
            const float b = 2.f / camera_size.y;
            const float c = -a * camera_center.x;
            const float d = -b * camera_center.y;
            const float view_proj_matrix[16] = {
                a, 0.f, 0.f, 0.f,
                0.f, b, 0.f, 0.f,
                0.f, 0.f, 1.f, 0.f,
                c, d, 0.f, 1.f
            };
            graphics::FrameUniformBlock frame_ub{};
            frame_ub.engine_time = _time;
            frame_ub.game_time = _game_time;
            frame_ub.window_framebuffer_width = (float)window_framebuffer_size.x;
            frame_ub.window_framebuffer_height = (float)window_framebuffer_size.y;
            memcpy(frame_ub.view_proj_matrix, view_proj_matrix, sizeof(view_proj_matrix));
            graphics::update_buffer(graphics::frame_uniform_buffer, &frame_ub, sizeof(frame_ub));
        }

        graphics::clear_framebuffer(graphics::game_ping_framebuffer);
        // Try to ensure game_ping_framebuffer is unbound as input before binding it as output
        graphics::bind_texture(0, Handle<graphics::Texture>());
        graphics::bind_framebuffer(graphics::game_ping_framebuffer);
        graphics::set_viewport({ .width = camera_size.x, .height = camera_size.y });

        // RENDER SPRITES TO GAME FRAMEBUFFER

        sprites::clear_statistics();
        background::draw_sprites_now(camera_min, camera_max);
        ecs::draw_sprites_now(camera_min, camera_max);

        // POSTPROCESS GAME FRAMEBUFFER

        switch (ui::get_top_menu()) {
            case ui::MenuType::Pause:
            case ui::MenuType::Settings:
            case ui::MenuType::Credits: {
                postprocessing::set_gaussian_blur_iterations(2);
            } break;
            default: {
                postprocessing::set_gaussian_blur_iterations(0);
            } break;
        }

        postprocessing::set_darkness_intensity(map::is_dark() ? 0.95f : 0.f);
        postprocessing::set_screen_transition_progress(map::get_transition_progress());
        postprocessing::render(camera_min, camera_max);

        // UPSCALE GAME FRAMEBUFFER TO FINAL FRAMEBUFFER

        if (!window::minimized()) {
            graphics::ScopedDebugGroup debug_group("Upscale to final framebuffer");
            graphics::clear_framebuffer(graphics::final_framebuffer);
            graphics::bind_framebuffer(graphics::final_framebuffer);
            graphics::bind_vertex_shader(graphics::fullscreen_vert);
            graphics::bind_fragment_shader(graphics::fullscreen_frag);
            graphics::bind_texture(0, graphics::get_framebuffer_texture(graphics::game_ping_framebuffer));
            graphics::set_viewport(viewport);
            graphics::set_primitives(graphics::Primitives::TriangleList);
            graphics::draw(3); // draw a fullscreen-covering triangle
        }

#ifdef _DEBUG
        ecs::debug_draw({ camera_min, camera_max });

        // RENDER DEBUG SHAPES TO FINAL FRAMEBUFFER

        shapes::draw_all_now("shapes::draw_all() [ECS debug]", camera_min, camera_max);
        shapes::update_lifetimes(_game_delta_time);
#endif

        // RENDER UI TO FINAL FRAMEBUFFER

        ui::update(_delta_time);
        ui::layout();
        ui::render();
        ui::render_rmlui(); // TODO: remove

        _render_copy_final_framebuffer_to_back_buffer();

        if (debug_stats) {
            _show_debug_stats_imgui();
        }

        // PITFALL: ImGui uses its own shaders and such, so we need to render it
        // to the back buffer, not the final framebuffer, since when using OpenGL
        // as backend we flip the final framebuffer vertically.
        imgui_impl::render();
        graphics::present_swap_chain_back_buffer();
        renderdoc::open_capture_directory_if_frame_capturing();
    }

    void run() {
        _update();
        _render();
    }
}