#pragma once
#include "graphics.h"
#include "graphics_vertices.h"

namespace graphics {

	struct FrameUniformBlock {
		float engine_time = 0.f;
		float game_time = 0.f;
		float viewport_width = 0.f;
		float viewport_height = 0.f;
		float view_proj_matrix[16] = {};
	};

	struct UiUniformBlock { // TODO: remove
		float transform[16] = {};
	};

	struct PlayerOutfitUniformBlock {
		int lut1_type = -1;
		int lut1_y = -1;
		int lut2_type = -1;
		int lut2_y = -1;
	};

	// You may use this as a temporary scratch buffer for vertices, for example
	// when building a list of vertices to eventually upload to a buffer.
	extern eastl::vector<graphics::VertexPCT> temp_vertices;

	// SHADERS

	extern Handle<VertexShader> fullscreen_vert;
	extern Handle<VertexShader> fullscreen_flip_vert;
	extern Handle<FragmentShader> fullscreen_frag;
	extern Handle<VertexShader> sprite_vert;
	extern Handle<FragmentShader> sprite_frag;
	extern Handle<VertexShader> shape_vert;
	extern Handle<FragmentShader> shape_frag;
	extern Handle<FragmentShader> text_frag;
	extern Handle<VertexShader> ui_vert;
	extern Handle<FragmentShader> ui_frag;
	extern Handle<FragmentShader> player_outfit_frag;

	// VERTEX INPUTS

	extern Handle<VertexInput> vertex_pc_input;
	extern Handle<VertexInput> vertex_pct_input;

	// BUFFERS

	extern Handle<Buffer> dynamic_vertex_buffer;
	extern Handle<Buffer> dynamic_index_buffer;
	extern Handle<Buffer> frame_uniform_buffer;
	extern Handle<Buffer> ui_uniform_buffer;
	extern Handle<Buffer> sprite_uniform_buffer;
	extern Handle<Buffer> player_outfit_uniform_buffer;

	// TEXTURES

	extern Handle<Texture> error_texture;
	extern Handle<Texture> white_texture; // 1x1 white texture

	// SAMPLERS

	extern Handle<Sampler> nearest_sampler; // nearest filtering, wrap
	extern Handle<Sampler> linear_sampler; // linear filtering, wrap

	// FRAMEBUFFERS

	extern Handle<Framebuffer> big_ping_framebuffer; // same size as the window framebuffer
	extern Handle<Framebuffer> big_pong_framebuffer; // same size as the window framebuffer
	extern Handle<Framebuffer> small_ping_framebuffer; // GAME_FRAMEBUFFER_WIDTH x GAME_FRAMEBUFFER_HEIGHT
	extern Handle<Framebuffer> small_pong_framebuffer; // GAME_FRAMEBUFFER_WIDTH x GAME_FRAMEBUFFER_HEIGHT
	extern Handle<Framebuffer> player_outfit_framebuffer; // 1024 x 1024

	// RASTERIZER STATES

	extern Handle<RasterizerState> default_rasterizer_state;

	// BLEND STATES

	extern Handle<BlendState> default_blend_state;

	void startup_globals();
	void resize_big_framebuffers(const Vec2u& size);
}