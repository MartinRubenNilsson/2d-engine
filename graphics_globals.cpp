#include "stdafx.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"
#include "files.h"

namespace graphics {
	eastl::vector<graphics::Vertex> temp_vertices;

	Handle<VertexShader> fullscreen_vert;
	Handle<VertexShader> fullscreen_flip_vert;
	Handle<FragmentShader> fullscreen_frag;
	Handle<VertexShader> sprite_vert;
	Handle<FragmentShader> sprite_frag;
	Handle<VertexShader> shape_vert;
	Handle<FragmentShader> shape_frag;
	Handle<FragmentShader> text_frag;
	Handle<VertexShader> ui_vert;
	Handle<FragmentShader> ui_frag;
	Handle<FragmentShader> player_outfit_frag;

	Handle<VertexInput> sprite_vertex_input;

	Handle<Buffer> dynamic_vertex_buffer;
	Handle<Buffer> dynamic_index_buffer;
	Handle<Buffer> frame_uniform_buffer;
	Handle<Buffer> ui_uniform_buffer;
	Handle<Buffer> sprite_uniform_buffer;
	Handle<Buffer> player_outfit_uniform_buffer;

	Handle<Texture> error_texture; // 32x32 magenta-black checkerboard
	Handle<Texture> white_texture; // 1x1 white texture

	Handle<Sampler> nearest_sampler;
	Handle<Sampler> linear_sampler;

	Handle<Framebuffer> final_framebuffer;
	Handle<Framebuffer> game_ping_framebuffer;
	Handle<Framebuffer> game_pong_framebuffer;
	Handle<Framebuffer> player_outfit_framebuffer;

	Handle<RasterizerState> default_rasterizer_state;

	Handle<BlendState> default_blend_state;

	void _load_and_create_shaders_and_vertex_inputs() {
		std::vector<unsigned char> shader_code;
#ifdef GRAPHICS_API_OPENGL
		const bool binary = graphics::is_spirv_supported();
		const std::string extension = binary ? ".spv" : "";
#endif
#ifdef GRAPHICS_API_D3D11
		const bool binary = true;
		const std::string extension = binary ? ".dxbc" : ".hlsl";
#endif
		if (files::read_binary_file("assets/shaders/fullscreen.vert" + extension, shader_code)) {
			fullscreen_vert = create_vertex_shader({
				.debug_name = "fullscreen vertex shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/fullscreen_flip.vert" + extension, shader_code)) {
			fullscreen_flip_vert = create_vertex_shader({
				.debug_name = "fullscreen flip vertex shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/fullscreen.frag" + extension, shader_code)) {
			fullscreen_frag = create_fragment_shader({
				.debug_name = "fullscreen fragment shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/sprite.vert" + extension, shader_code)) {
			sprite_vert = create_vertex_shader({
				.debug_name = "sprite vertex shader",
				.code = shader_code,
				.binary = binary
			});
			sprite_vertex_input = graphics::create_vertex_input({
				.debug_name = "sprite vertex input",
				.attributes = { {
					.format = Format::RG32_FLOAT,
					.offset = offsetof(Vertex, position)
				}, {
					.format = Format::RGBA8_UNORM,
					.offset = offsetof(Vertex, color),
					.normalized = true // FIXME: normalized is not supported in d3d11
				}, {
					.format = Format::RG32_FLOAT,
					.offset = offsetof(Vertex, tex_coord)
				}, },
				.bytecode = shader_code
			});
		}
		if (files::read_binary_file("assets/shaders/sprite.frag" + extension, shader_code)) {
			sprite_frag = create_fragment_shader({
				.debug_name = "sprite fragment shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/shape.vert" + extension, shader_code)) {
			shape_vert = create_vertex_shader({
				.debug_name = "shape vertex shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/shape.frag" + extension, shader_code)) {
			shape_frag = create_fragment_shader({
				.debug_name = "shape fragment shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/text.frag" + extension, shader_code)) {
			text_frag = create_fragment_shader({
				.debug_name = "text fragment shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/ui.vert" + extension, shader_code)) {
			ui_vert = create_vertex_shader({
				.debug_name = "ui vertex shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/ui.frag" + extension, shader_code)) {
			ui_frag = create_fragment_shader({
				.debug_name = "ui fragment shader",
				.code = shader_code,
				.binary = binary
			});
		}
		if (files::read_binary_file("assets/shaders/player_outfit.frag" + extension, shader_code)) {
			player_outfit_frag = create_fragment_shader({
				.debug_name = "player outfit fragment shader",
				.code = shader_code,
				.binary = binary
			});
		}
	}

	void _create_buffers() {
		dynamic_vertex_buffer = create_buffer({
			.debug_name = "dynamic vertex buffer",
			.size = 8192 * sizeof(Vertex), // 8192 is an initial estimate
			.type = BufferType::VertexBuffer,
			.dynamic = true
		});
		dynamic_index_buffer = create_buffer({
			.debug_name = "dynamic index buffer",
			.size = 8192 * sizeof(unsigned int), // 8192 is an initial estimate
			.type = BufferType::IndexBuffer,
			.dynamic = true
		});
		frame_uniform_buffer = create_buffer({
			.debug_name = "frame uniform buffer",
			.size = sizeof(FrameUniformBlock),
			.type = BufferType::UniformBuffer,
			.dynamic = true
		});
		ui_uniform_buffer = create_buffer({
			.debug_name = "ui uniform buffer",
			.size = sizeof(UiUniformBlock),
			.type = BufferType::UniformBuffer,
			.dynamic = true
		});
		sprite_uniform_buffer = create_buffer({
			.debug_name = "sprite uniform buffer",
			.size = 256 * 256, // estimate we won't need more than 256 custom blocks on screen at same time
			.type = BufferType::UniformBuffer,
			.dynamic = true
		});
		player_outfit_uniform_buffer = create_buffer({
			.debug_name = "player outfit uniform buffer",
			.size = sizeof(PlayerOutfitUniformBlock),
			.type = BufferType::UniformBuffer,
			.dynamic = true
		});
	}

	void _create_textures() {
		{
			constexpr unsigned int ERROR_TEXTURE_SIZE = 32;
			unsigned char error_texture_data[ERROR_TEXTURE_SIZE * ERROR_TEXTURE_SIZE * 4];
			//Make a magenta-and-black checkerboard pattern
			for (unsigned int i = 0; i < ERROR_TEXTURE_SIZE; ++i) {
				for (unsigned int j = 0; j < ERROR_TEXTURE_SIZE; ++j) {
					unsigned int k = (i * ERROR_TEXTURE_SIZE + j) * 4;
					unsigned char value = (i + j) % 2 == 0 ? 255 : 0;
					error_texture_data[k + 0] = value;
					error_texture_data[k + 1] = 0;
					error_texture_data[k + 2] = value;
					error_texture_data[k + 3] = 255;
				}
			}
			error_texture = create_texture({
				.debug_name = "error texture",
				.width = ERROR_TEXTURE_SIZE,
				.height = ERROR_TEXTURE_SIZE,
				.format = Format::RGBA8_UNORM,
				.initial_data = error_texture_data
			});
		}
		{
			unsigned char white_texture_data[4] = { 255, 255, 255, 255 };
			white_texture = create_texture({
				.debug_name = "white texture",
				.width = 1,
				.height = 1,
				.format = Format::RGBA8_UNORM,
				.initial_data = white_texture_data
			});
		}
	}

	void _create_samplers() {
		nearest_sampler = create_sampler({
			.debug_name = "nearest sampler",
			.filter = Filter::Nearest
		});
		linear_sampler = create_sampler({
			.debug_name = "linear sampler",
			.filter = Filter::Linear,
			.wrap = Wrap::ClampToEdge
		});
	}

	void _create_framebuffers() {
		final_framebuffer = create_framebuffer({
			.debug_name = "final framebuffer"
		});
		attach_framebuffer_texture(final_framebuffer, create_texture({
			.debug_name = "final framebuffer color texture",
			// this is just the initial size, it will be resized when the window is resized
			.width = GAME_FRAMEBUFFER_WIDTH,
			.height = GAME_FRAMEBUFFER_HEIGHT,
			.format = Format::RGBA8_UNORM,
			.framebuffer_color = true
		}));

		game_ping_framebuffer = create_framebuffer({
			.debug_name = "game ping framebuffer"
		});
		attach_framebuffer_texture(game_ping_framebuffer, create_texture({
			.debug_name = "game ping framebuffer color texture",
			.width = GAME_FRAMEBUFFER_WIDTH,
			.height = GAME_FRAMEBUFFER_HEIGHT,
			.format = Format::RGBA8_UNORM,
			.framebuffer_color = true
		}));

		game_pong_framebuffer = create_framebuffer({
			.debug_name = "game pong framebuffer"
		});
		attach_framebuffer_texture(game_pong_framebuffer, create_texture({
			.debug_name = "game pong framebuffer color texture",
			.width = GAME_FRAMEBUFFER_WIDTH,
			.height = GAME_FRAMEBUFFER_HEIGHT,
			.format = Format::RGBA8_UNORM,
			.framebuffer_color = true
		}));

		player_outfit_framebuffer = create_framebuffer({
			.debug_name = "player outfit framebuffer"
		});
		attach_framebuffer_texture(player_outfit_framebuffer, create_texture({
			.debug_name = "player outfit framebuffer color texture",
			.width = 1024, // size of player tileset
			.height = 1024, // size of player tileset
			.format = Format::RGBA8_UNORM,
			.framebuffer_color = true
		}));
	}

	void _create_rasterizer_states() {
		default_rasterizer_state = create_rasterizer_state({
			.debug_name = "default rasterizer state",
			.cull_mode = CullMode::None
		});
	}

	void _create_blend_states() {
		default_blend_state = create_blend_state({
			.debug_name = "default blend state",
			.attachments = { {
				.blend_enable = true,
				.src_color_blend_factor = BlendFactor::SrcAlpha,
				.dst_color_blend_factor = BlendFactor::OneMinusSrcAlpha,
				.color_blend_op = BlendOp::Add,
				.src_alpha_blend_factor = BlendFactor::One,
				.dst_alpha_blend_factor = BlendFactor::OneMinusSrcAlpha,
				.alpha_blend_op = BlendOp::Add,
			} }
		});
	}

	void _bind_globals() {
		bind_vertex_input(sprite_vertex_input);
		bind_vertex_buffer(0, dynamic_vertex_buffer, sizeof(Vertex));
		bind_index_buffer(dynamic_index_buffer);
		bind_uniform_buffer(0, frame_uniform_buffer);
		bind_sampler(0, nearest_sampler);
		bind_rasterizer_state(default_rasterizer_state);
		bind_blend_state(default_blend_state);
	}

	void startup_globals() {
		_load_and_create_shaders_and_vertex_inputs();
		_create_framebuffers();
		_create_buffers();
		_create_textures();
		_create_samplers();
		_create_rasterizer_states();
		_create_blend_states();
		_bind_globals();
	}

	void resize_final_framebuffer(unsigned int new_width, unsigned int new_height) {
		graphics::resize_framebuffer(final_framebuffer, new_width, new_height);
	}
}