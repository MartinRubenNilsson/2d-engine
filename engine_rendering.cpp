#include "stdafx.h"
#include "engine_fps_counter.h"
#include "graphics.h"
#include "graphics_debugging.h"
#include "graphics_globals.h"
#include "window.h"
#include "sprites.h"
#include "shapes.h"
#include "postprocessing.h"
#include "ecs.h"
#include "map.h"
#include "imgui_impl.h"
#include "ui.h"
#include "ui_game.h"
#include "background.h"

namespace engine {
    extern double _time;
    extern double _game_time;
    extern double _delta_time;

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

    void _render_to_big_framebuffer() {
        GRAPHICS_DEBUG_GROUP;
        const Handle<graphics::Framebuffer> prev_framebuffer = graphics::get_bound_framebuffer();
        graphics::clear_framebuffer(graphics::big_ping_framebuffer);
        graphics::bind_framebuffer(graphics::big_ping_framebuffer);
        graphics::bind_vertex_shader(graphics::fullscreen_vert);
        graphics::bind_fragment_shader(graphics::fullscreen_frag);
        graphics::bind_texture(0, graphics::get_framebuffer_texture(prev_framebuffer));
        const Vec2u new_framebuffer_size = graphics::get_texture_size(
            graphics::get_framebuffer_texture(graphics::big_ping_framebuffer));
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

        graphics::clear_framebuffer(graphics::small_ping_framebuffer);
        // Try to ensure game_ping_framebuffer is unbound as input before binding it as output
        graphics::bind_texture(0, Handle<graphics::Texture>());
        graphics::bind_framebuffer(graphics::small_ping_framebuffer);
        graphics::set_viewport({ .width = view.max.x - view.min.x, .height = view.max.y - view.min.y });

        _update_frame_uniform_block(view);

        sprites::clear_statistics();
        background::draw_sprites_now(view);
        ecs::draw_sprites_now(view);

        if (ui::game::should_blur_background()) {
            postprocessing::set_gaussian_blur_iterations(2);
        } else {
            postprocessing::set_gaussian_blur_iterations(0);
        }

        postprocessing::set_darkness_intensity(map::is_dark() ? 0.95f : 0.f);
        postprocessing::set_screen_transition_progress(map::get_transition_progress());

        postprocessing::render_pre_ui(view);

        _render_to_big_framebuffer();

        ecs::debug_draw(view);

        shapes::draw_all_now();
        shapes::update_lifetimes((float)_delta_time);

        _update_frame_uniform_block({ .max = { GAME_FRAMEBUFFER_WIDTH, GAME_FRAMEBUFFER_HEIGHT } });

        ui::begin_layout();
        ui::game::layout();
        ui::end_layout();
        ui::render();

        postprocessing::render_post_ui(view);

        _render_to_back_buffer();

        if (should_show_fps_counter) {
            show_fps_counter();
        }

        // PITFALL: ImGui uses its own shaders and such, so we need to render it to the back buffer, not the
        // final framebuffer, since when using OpenGL as backend we flip the final framebuffer vertically.
        imgui_impl::render();
        graphics::present_swap_chain_back_buffer();
    }
}