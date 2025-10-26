#include "stdafx.h"
#include "engine_fps_counter.h"
#include "steam.h"
#include "files.h"
#include "networking.h"
#include "window.h"
#include "window_events.h"
#include "audio.h"
#include "ui_rmlui.h"
#include "ui_menus.h"
#include "ui.h"
#include "map.h"
#include "ecs.h"
#include "console.h"
#include "background.h"
#include "postprocessing.h"
#include "settings.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_debugging.h"
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
        ecs::startup();
        settings::load_from_file(settings::APP_SETTINGS_PATH, settings::app_settings);
        settings::apply(settings::app_settings);
        window::set_visible(true);
        console::execute(argc, argv);
        _should_run = true;
    }

    void shutdown() {
        ecs::shutdown();
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
    double _game_delta_time = 0.0;

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
        imgui_impl::new_frame();
        console::handle_window_events();
        input::handle_window_events();

        if (window::should_close()) {
            _should_run = false;
        }

        // PROCESS WINDOW EVENTS
        {
            for (const window::Event& ev : window::get_events()) {
                if (ev.type == window::EventType::FramebufferSize) {
                    // When the window is minimized, an event is sent with size 0, 0,
                    // which we must therefore ignore.
                    if (ev.size.width && ev.size.height) {
                        graphics::resize_swap_chain_framebuffer({ (unsigned int)ev.size.width, (unsigned int)ev.size.height });
                        graphics::resize_final_framebuffer(ev.size.width, ev.size.height);
                    }
                } else if (ev.type == window::EventType::KeyPress) {
                    if (ev.key.code == window::Key::F7) {
                        map::debug = !map::debug;
                    }
                }
            }
        }

        audio::update();
        console::update((float)_delta_time);

        if (map::is_open()) {
            background::set_type(background::Type::None);
        } else {
            background::set_type(background::Type::MountainDusk);
            background::update((float)_delta_time); // TODO: this doesn't belong in engine.cpp
        }

        map::update((float)_delta_time); // TODO: this doesn't belong in engine.cpp

        double game_delta_time = _delta_time;
        if (steam::is_overlay_active()) {
            game_delta_time = 0.0;
        }
        if (ui::is_menu_or_visible()) { // TODO: this doesn't belong in engine.cpp
            game_delta_time = 0.0;
        }
        if (map::get_transition_progress() != 0.f) {
            game_delta_time = 0.0; // pause game while map is transitioning
        }

        _game_time += game_delta_time;

        ecs::update((float)game_delta_time);
        ui::update((float)_delta_time);
        postprocessing::update((float)game_delta_time);
    }

    void _update_frame_uniform_block(const Rect2f& view) {
        GRAPHICS_DEBUG_GROUP;

        graphics::FrameUniformBlock block{};
        block.engine_time = (float)_time;
        block.game_time = (float)_game_time;
        {
            const graphics::Viewport& viewport = graphics::get_viewport();
            block.viewport_width = viewport.width;
            block.viewport_height = viewport.height;
        }
        {
            const Vec2f view_center = (view.min + view.max) * 0.5f;
            const Vec2f view_size = view.max - view.min;
            // PITFALL: We use an unusual clip space coordinate system where y is down.
            // This makes it easier to handle some differences between OpenGL and D3D11.
            // Moreover, it means the shader coordinate axes point the same as the game world.
            const float a = 2.f / view_size.x;
            const float b = 2.f / view_size.y;
            const float c = -a * view_center.x;
            const float d = -b * view_center.y;
            const float view_proj_matrix[16] = {
                a, 0.f, 0.f, 0.f,
                0.f, b, 0.f, 0.f,
                0.f, 0.f, 1.f, 0.f,
                c, d, 0.f, 1.f
            };
            memcpy(block.view_proj_matrix, view_proj_matrix, sizeof(view_proj_matrix));
        }

        graphics::update_buffer(graphics::frame_uniform_buffer, &block, sizeof(block));
    }

    void _render_to_final_framebuffer() {
        GRAPHICS_DEBUG_GROUP;
        const Handle<graphics::Framebuffer> prev_framebuffer = graphics::get_bound_framebuffer();
        graphics::clear_framebuffer(graphics::final_framebuffer);
        graphics::bind_framebuffer(graphics::final_framebuffer);
        graphics::bind_vertex_shader(graphics::fullscreen_vert);
        graphics::bind_fragment_shader(graphics::fullscreen_frag);
        graphics::bind_texture(0, graphics::get_framebuffer_texture(prev_framebuffer));
        const Vec2u new_framebuffer_size = graphics::get_texture_size(
            graphics::get_framebuffer_texture(graphics::final_framebuffer));
        graphics::set_viewport({ .width = (float)new_framebuffer_size.x, .height = (float)new_framebuffer_size.y });
        graphics::set_primitives(graphics::Primitives::TriangleList);
        graphics::draw(3); // draw a fullscreen-covering triangle
    }

    void _render_to_back_buffer() {
        GRAPHICS_DEBUG_GROUP;
        const Handle<graphics::Framebuffer> prev_framebuffer = graphics::get_bound_framebuffer();
        const Handle<graphics::Framebuffer> new_framebuffer = graphics::get_swap_chain_back_buffer();
        graphics::clear_framebuffer(new_framebuffer);
        graphics::bind_framebuffer(new_framebuffer);
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
        graphics::bind_texture(0, graphics::get_framebuffer_texture(prev_framebuffer));
        graphics::bind_sampler(0, graphics::nearest_sampler);
#if 0
        const Vec2u new_framebuffer_size = graphics::get_texture_size(
            graphics::get_framebuffer_texture(new_framebuffer));
        graphics::set_viewport({ .width = (float)new_framebuffer_size.x, .height = (float)new_framebuffer_size.y });
#endif
        graphics::set_primitives(graphics::Primitives::TriangleList);
        graphics::draw(3); // draw a fullscreen-covering triangle
    }

    void _render() {
        const Vec2i window_framebuffer_size = window::get_framebuffer_size();
        const graphics::Viewport viewport = {
            .width = (float)window_framebuffer_size.x,
            .height = (float)window_framebuffer_size.y };

        Rect2f view{}; // The current camera view in world-space.
        view.min = Vec2f::ZERO;
        view.max = window_framebuffer_size; // Default
        if (map::is_open()) {
            view = ecs::get_camera_view();
        }

        graphics::clear_framebuffer(graphics::game_ping_framebuffer);
        // Try to ensure game_ping_framebuffer is unbound as input before binding it as output
        graphics::bind_texture(0, Handle<graphics::Texture>());
        graphics::bind_framebuffer(graphics::game_ping_framebuffer);
        graphics::set_viewport({ .width = view.max.x - view.min.x, .height = view.max.y - view.min.y });

        _update_frame_uniform_block(view);

        sprites::clear_statistics();
        background::draw_sprites_now(view);
        ecs::draw_sprites_now(view);

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
        postprocessing::render(view);

        _render_to_final_framebuffer();

        ecs::debug_draw(view);

        shapes::draw_all_now();
        shapes::update_lifetimes((float)_game_delta_time);

        _update_frame_uniform_block({ .max = { GAME_FRAMEBUFFER_WIDTH, GAME_FRAMEBUFFER_HEIGHT } });

        ui::layout();
        ui::render();

        _render_to_back_buffer();

        if (should_show_fps_counter) {
            show_fps_counter();
        }

        // PITFALL: ImGui uses its own shaders and such, so we need to render it to the back buffer, not the
        // final framebuffer, since when using OpenGL as backend we flip the final framebuffer vertically.
        imgui_impl::render();
        graphics::present_swap_chain_back_buffer();
        renderdoc::open_capture_directory_if_frame_capturing();
    }

    void run() {
        _update();
        _render();
    }
}