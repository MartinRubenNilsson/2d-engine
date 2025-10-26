#pragma once

// graphics_api.h - Low-level graphics API

#include "graphics_api_config.h"
#include "graphics_types.h"
#include <span>

#ifdef GRAPHICS_API_D3D11
struct ID3D11Device;
struct ID3D11DeviceContext;
#endif

namespace graphics {
namespace api {

	using DebugMessageCallback = void(*)(std::string_view message);

	void set_debug_message_callback(DebugMessageCallback callback);

	struct StartupOptions {
		const char* application_name = "Application"; // Only needed with GRAPHICS_API_VULKAN.
		const char* engine_name = "Engine"; // Only needed with GRAPHICS_API_VULKAN.
		void* (*glad_load_proc)(const char* name) = nullptr; // Only needed with GRAPHICS_API_OPENGL.
		std::span<const char*> vulkan_instance_extensions; // Only needed with GRAPHICS_API_VULKAN.
		void* hwnd = nullptr; // Win32 window handle (HWND). Only needed with GRAPHICS_API_D3D11.
	};

	bool startup(const StartupOptions &options);
	void shutdown();

	bool is_spirv_supported();

#ifdef GRAPHICS_API_D3D11
	ID3D11Device* get_d3d11_device();
	ID3D11DeviceContext* get_d3d11_device_context();
	bool resize_swap_chain_framebuffer(unsigned int new_width, unsigned int new_height);
	void present_swap_chain_back_buffer(unsigned int sync_interval = 0);
#endif

	void push_debug_group(std::string_view name);
	void pop_debug_group();

	struct VertexShaderId { uintptr_t object = 0; };

	VertexShaderId create_vertex_shader(const ShaderDesc& desc);
	void destroy_vertex_shader(VertexShaderId shader);
	void bind_vertex_shader(VertexShaderId shader);

	struct FragmentShaderId { uintptr_t object = 0; };

	FragmentShaderId create_fragment_shader(const ShaderDesc& desc);
	void destroy_fragment_shader(FragmentShaderId shader);
	void bind_fragment_shader(FragmentShaderId shader);

	struct VertexInputId { uintptr_t object = 0; };

	VertexInputId create_vertex_input(const VertexInputDesc& desc);
	void destroy_vertex_input(VertexInputId vertex_input);
	void bind_vertex_input(VertexInputId vertex_input);

	struct BufferId { uintptr_t object = 0; };

	BufferId create_buffer(const BufferDesc& desc);
	void destroy_buffer(BufferId buffer);
	void update_buffer(BufferId buffer, const void* data, unsigned int size, unsigned int offset);
	void bind_uniform_buffer(unsigned int binding, BufferId buffer);
	void bind_uniform_buffer_range(unsigned int binding, BufferId buffer, unsigned int size, unsigned int offset);
	void bind_vertex_buffer(unsigned int binding, BufferId buffer, unsigned int stride, unsigned int offset);
	void bind_index_buffer(BufferId buffer, unsigned int offset = 0);

	struct TextureId { uintptr_t object = 0; };

	TextureId create_texture(const TextureDesc& desc);
	void destroy_texture(TextureId texture);
	void update_texture(
		TextureId texture,
		unsigned int level,
		unsigned int x,
		unsigned int y,
		unsigned int width,
		unsigned int height,
		Format pixel_format,
		const void* pixels);
	void copy_texture(
		TextureId dst_texture,
		unsigned int dst_level,
		unsigned int dst_x,
		unsigned int dst_y,
		unsigned int dst_z,
		TextureId src_texture,
		unsigned int src_level,
		unsigned int src_x,
		unsigned int src_y,
		unsigned int src_z,
		unsigned int src_width,
		unsigned int src_height,
		unsigned int src_depth);
	void bind_texture(unsigned int binding, TextureId texture);

	struct SamplerId { uintptr_t object = 0; };

	SamplerId create_sampler(const SamplerDesc& desc);
	void destroy_sampler(SamplerId sampler);
	void bind_sampler(unsigned int binding, SamplerId sampler);

	struct FramebufferId { uintptr_t object = 0; };

	FramebufferId get_swap_chain_back_buffer(); // aka the "default framebuffer"
	FramebufferId create_framebuffer(const FramebufferDesc& desc);
	void destroy_framebuffer(FramebufferId framebuffer);
	bool attach_framebuffer_color_texture(FramebufferId framebuffer, unsigned int attachment, TextureId texture);
	void clear_framebuffer_color(FramebufferId framebuffer, unsigned int attachment, const float color[4]);
	void bind_framebuffer(FramebufferId framebuffer);

	struct RasterizerStateId { uintptr_t object = 0; };

	RasterizerStateId create_rasterizer_state(const RasterizerDesc& desc);
	void destroy_rasterizer_state(RasterizerStateId state);
	void bind_rasterizer_state(RasterizerStateId state);

	struct BlendStateId { uintptr_t object = 0; };

	BlendStateId create_blend_state(const BlendDesc& desc);
	void destroy_blend_state(BlendStateId state);
	void bind_blend_state(BlendStateId state);

	void set_viewports(const Viewport* viewports, unsigned int count);
	void set_scissors(const Rect* scissors, unsigned int count);
	void set_scissor_test_enabled(bool enable);
	void set_primitives(Primitives primitives);

	void draw(unsigned int vertex_count, unsigned int vertex_offset = 0);
	void draw_indexed(unsigned int index_count, unsigned int base_vertex = 0);

} // namespace api
} // namespace graphics