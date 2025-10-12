#include "stdafx.h"
#include "steam.h"
#include "platform.h"
#include "filesystem.h"
#include "networking.h"
#include "window.h"
#include "window_events.h"
#include "audio.h"
#include "ui.h"
#include "ui_hud.h"
#include "ui_menus.h"
#include "ui_textbox.h"
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

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "en_US.utf8");
    if (steam::restart_app_if_necessary()) {
        return EXIT_FAILURE;
    }
    steam::initialize(); // Fails silently if Steam is not running.
    filesystem::initialize();
	networking::initialize();
#ifdef _DEBUG_RENDERDOC
    renderdoc::initialize();
#endif
    window::startup();
#if 0
#ifdef _DEBUG_GRAPHICS
    // HACK: We should be using a post-build event to copy the shaders,
    // but then it doesn't run when only debugging and not recompiling,
    // which is annoying when you've changed a shader but not the code,
    // because then the new shader doesn't get copied.
    platform::system("del /q assets\\shaders\\*");
#ifdef GRAPHICS_API_OPENGL
    platform::system("copy /Y ..\\shaders\\glsl\\* assets\\shaders");
#endif
#ifdef GRAPHICS_API_D3D11
	platform::system("copy /Y ..\\shaders\\dxbc\\* assets\\shaders");
#endif
#endif
#endif
    graphics::initialize();
    graphics::initialize_globals();
#ifdef _DEBUG_IMGUI
    imgui_impl::initialize();
#endif
    console::initialize();
    audio::initialize();
    ui::initialize();
    ecs::startup();

    for (const filesystem::File& file : filesystem::get_all_files_in_directory("assets/audio/banks")) {
        if (file.format != filesystem::FileFormat::FmodStudioBank) continue;
        audio::load_bank_from_file(file.path);
    }
    for (const filesystem::File& file : filesystem::get_all_files_in_directory("assets/fonts")) {
        if (file.format != filesystem::FileFormat::TrueTypeFont) continue;
        ui::load_font_from_file(file.path);
    }
    for (const filesystem::File& file : filesystem::get_all_files_in_directory("assets/ui")) {
        if (file.format != filesystem::FileFormat::RmlUiDocument) continue;
        ui::load_document_from_file(file.path);
    }

    ui::add_event_listeners(); // Must come after loading RML documents.

    settings::load_from_file(settings::APP_SETTINGS_PATH, settings::app_settings);
    settings::apply(settings::app_settings);

    window::set_visible(true);

    // PREPARE FOR GAME LOOP

#ifdef _DEBUG
    console::execute(argc, argv);
#else
    console::execute("execute_script assets/scripts/martin_debug.script");
#endif

    bool debug_stats = false;
    bool debug_textures = false;

    // GAME LOOP

    float app_time = (float)window::get_elapsed_time();
	float game_time = 0.0;

    while (!window::should_close()) {

        const float new_app_time = (float)window::get_elapsed_time();
        const float app_delta_time = new_app_time - app_time;
        app_time = new_app_time;

        steam::run_message_loop();
        window::update_events();

#ifdef _DEBUG_IMGUI
        // TODO: should this be moved to after the window event loop?
        imgui_impl::new_frame();
#endif

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
#ifdef _DEBUG
                    if (ev.key.code == window::Key::GraveAccent) {
                        console::toggle_visible();
                    } else if (ev.key.code == window::Key::F1) {
                        debug_stats = !debug_stats;
                    } else if (ev.key.code == window::Key::F6) {
                        ui::debug = !ui::debug;
                    } else if (ev.key.code == window::Key::F7) {
                        map::debug = !map::debug;
                    } else if (ev.key.code == window::Key::F8) {
                        debug_textures = !debug_textures;
                    }
#endif // _DEBUG
                }

#ifdef _DEBUG_IMGUI
                if (ev.type == window::EventType::KeyPress && ImGui::GetIO().WantCaptureKeyboard) {
                    continue;
                }
#endif
                console::process_window_event(ev);
                ui::process_window_event(ev);
            }
        }

        // PROCESS UI EVENTS
        {
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
        console::update(app_delta_time);
        background::update(app_delta_time);
        ui::update(app_delta_time);
        map::update(app_delta_time);

        float game_delta_time = app_delta_time;
        if (steam::is_overlay_active()) {
            game_delta_time = 0.0;
        }
        if (ui::is_menu_or_textbox_visible()) {
            game_delta_time = 0.0;
        }
        if (map::get_transition_progress() != 0.f) {
            game_delta_time = 0.0; // pause game while map is transitioning
        }

        game_time += game_delta_time;

        ecs::update(game_delta_time);
        postprocessing::update(game_delta_time);

        // RENDER

        sprites::clear_statistics();

        int window_framebuffer_width = 0;
        int window_framebuffer_height = 0;
        window::get_framebuffer_size(window_framebuffer_width, window_framebuffer_height);

        Vec2f camera_min = { 0.f, 0.f };
		Vec2f camera_max = { (float)window_framebuffer_width, (float)window_framebuffer_height };
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
				0.f,   b, 0.f, 0.f,
				0.f, 0.f, 1.f, 0.f,
				  c,   d, 0.f, 1.f
			};
            graphics::FrameUniformBlock frame_ub{};
			frame_ub.app_time = app_time;
			frame_ub.game_time = game_time;
			frame_ub.window_framebuffer_width = (float)window_framebuffer_width;
			frame_ub.window_framebuffer_height = (float)window_framebuffer_height;
            memcpy(frame_ub.view_proj_matrix, view_proj_matrix, sizeof(view_proj_matrix));
            graphics::update_buffer(graphics::frame_uniform_buffer, &frame_ub, sizeof(frame_ub));
        }

		constexpr float CLEAR_COLOR[4] = { 0.f, 0.f, 0.f, 0.f };
        graphics::clear_framebuffer(graphics::game_ping_framebuffer, CLEAR_COLOR);
		// Try to ensure game_ping_framebuffer is unbound as input before binding it as output
        graphics::bind_texture(0, Handle<graphics::Texture>());
        graphics::bind_framebuffer(graphics::game_ping_framebuffer);
        graphics::set_viewport({ .width = camera_size.x, .height = camera_size.y });

		// RENDER SPRITES TO GAME FRAMEBUFFER

        background::draw_sprites(camera_min, camera_max);
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

		if (!window::get_minimized()) {
            graphics::ScopedDebugGroup debug_group("Upscale to final framebuffer");
			graphics::clear_framebuffer(graphics::final_framebuffer, CLEAR_COLOR);
			graphics::bind_framebuffer(graphics::final_framebuffer);
            graphics::bind_vertex_shader(graphics::fullscreen_vert);
            graphics::bind_fragment_shader(graphics::fullscreen_frag);
            graphics::bind_texture(0, graphics::get_framebuffer_texture(graphics::game_ping_framebuffer));
            graphics::set_viewport({ .width = (float)window_framebuffer_width, .height = (float)window_framebuffer_height });
			graphics::set_primitives(graphics::Primitives::TriangleList);
            graphics::draw(3); // draw a fullscreen-covering triangle
        }

#ifdef _DEBUG
        ecs::debug_draw();

		// RENDER DEBUG SHAPES TO FINAL FRAMEBUFFER

        shapes::draw_all("shapes::draw_all() [ECS debug]", camera_min, camera_max);
        shapes::update_lifetimes(game_delta_time);
#endif

		// RENDER UI TO FINAL FRAMEBUFFER

        ui::render();

		// COPY FINAL FRAMEBUFFER TO BACK BUFFER
        {
            graphics::ScopedDebugGroup debug_group("Copy to back buffer");
			const Handle<graphics::Framebuffer> back_buffer = graphics::get_swap_chain_back_buffer();
			graphics::clear_framebuffer(back_buffer, CLEAR_COLOR);
            graphics::bind_framebuffer(back_buffer);
#ifdef GRAPHICS_API_OPENGL
            // PITFALL: In order to easier handle some differences between OpenGL and D3D11,
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

#ifdef _DEBUG_IMGUI
        // CREATE DEBUG STATS WINDOW

        if (debug_stats) {
            constexpr float smoothing_factor = 0.99f;
            static float smoothed_dt = 0.f;
            static float smoothed_fps = 0.f;
            static float dt_buffer[256] = { 0.f };
            static float fps_buffer[256] = { 0.f };
            static int buffer_offset = 0;
            dt_buffer[buffer_offset] = app_delta_time;
            fps_buffer[buffer_offset] = 1.f / app_delta_time;
            buffer_offset = (buffer_offset + 1) % 256;
            smoothed_dt = smoothing_factor * smoothed_dt + (1.f - smoothing_factor) * app_delta_time;
            smoothed_fps = smoothing_factor * smoothed_fps + (1.f - smoothing_factor) / app_delta_time;
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
            char overlay_text[64];
            sprintf(overlay_text, "%.f us", smoothed_dt * 1'000'000.f);
            ImGui::PlotLines("##dt", dt_buffer, 256, buffer_offset, overlay_text, 0.f, 0.01f, ImVec2(0, 80));
            sprintf(overlay_text, "%.f FPS", smoothed_fps);
            ImGui::PlotLines("##fps", fps_buffer, 256, buffer_offset, overlay_text, 0.f, 600.f, ImVec2(0, 80));
            ImGui::Value("Sprites Drawn", sprites::get_num_sprites_drawn());
            ImGui::Value("Batches Drawn", sprites::get_num_batches_drawn());
            ImGui::Value("Largest Batch", sprites::get_num_sprites_in_largest_batch());
            ImGui::End();
        }
        if (debug_textures) {
            graphics::show_texture_debug_window();
        }
        {
            graphics::ScopedDebugGroup debug_group("imgui_impl::render()");
            // PITFALL: ImGui uses its own shaders and such, so we need to render it
			// to the back buffer, not the final framebuffer, since when using OpenGL
			// as backend we flip the final framebuffer vertically.
            imgui_impl::render();
        }
#endif

        // PRESENT BACK BUFFER

        graphics::present_swap_chain_back_buffer();

#ifdef _DEBUG_RENDERDOC
        if (renderdoc::is_frame_capturing()) {
			const std::string capture_file_directory =
                filesystem::get_parent_path(renderdoc::get_capture_file_path_template());
            platform::open(capture_file_directory.c_str());
        }
#endif
    }

    // SHUTDOWN

    ecs::shutdown();
    ui::shutdown();
    audio::shutdown();
#ifdef _DEBUG_IMGUI
    imgui_impl::shutdown();
#endif
    graphics::shutdown();
    window::shutdown();
	networking::shutdown();
    steam::shutdown();

    return EXIT_SUCCESS;
}