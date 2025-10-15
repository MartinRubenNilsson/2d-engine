#include "stdafx.h"
#include "graphics.h"
#include "graphics_api.h"
#include "graphics_vertices.h"
#include "window.h"
#include "window_graphics.h"
#include "pool.h"
#include "console.h"
#include "files.h"
#include "images.h"
#include "platform.h"

namespace graphics {

	// IMPORTANT: These structs are internal! Don't move them to graphics.h!

	struct VertexShader {
		api::VertexShaderHandle api_handle{};
	};

	struct FragmentShader {
		api::FragmentShaderHandle api_handle{};
	};

	struct VertexInput {
		api::VertexInputHandle api_handle{};
	};

	struct Buffer {
		api::BufferHandle api_handle{};
		BufferDesc desc{};
	};

	struct Texture {
		api::TextureHandle api_handle{};
		// PITFALL: When loading a texture, desc.debug_name is set to the texture path.
		// To ensure the string_view doesn't get invalidated, the owning string is
		// moved into _path_to_texture. If this map gets cleared for whatever reason,
		// then all these string_views will therefore be invalidated, so watch out!
		TextureDesc desc{};
	};

	struct Sampler {
		api::SamplerHandle api_handle{};
		SamplerDesc desc{};
	};

	struct Framebuffer {
		api::FramebufferHandle api_handle{};
		FramebufferDesc desc{};
		Handle<Texture> texture;
		bool is_swap_chain_back_buffer = false;
	};

	struct RasterizerState {
		api::RasterizerStateHandle api_handle{};
		RasterizerDesc desc{};
	};

	struct BlendState {
		api::BlendStateHandle api_handle{};
		BlendDesc desc{};
	};

	Viewport _viewport;
	Rect _scissor;
	bool _scissor_test_enabled = false;
	Pool<VertexShader> _vertex_shader_pool;
	Pool<FragmentShader> _fragment_shader_pool;
	Pool<VertexInput> _vertex_input_pool;
	Pool<Buffer> _buffer_pool;
	Pool<Texture> _texture_pool;
	std::unordered_map<std::string, Handle<Texture>> _path_to_texture;
	unsigned int _total_texture_memory_usage_in_bytes = 0;
	Pool<Sampler> _sampler_pool;
	Pool<Framebuffer> _framebuffer_pool;
	Handle<Framebuffer> _swap_chain_back_buffer_handle;
	Pool<RasterizerState> _rasterizer_state_pool;
	Pool<BlendState> _blend_state_pool;

#ifdef GRAPHICS_API_DEBUG
	void _debug_message_callback(std::string_view message) {
		__debugbreak();
		console::log_error(message);
	}
#endif

	bool startup() {
#ifdef GRAPHICS_API_DEBUG
		api::set_debug_message_callback(_debug_message_callback);
#endif
#ifdef GRAPHICS_API_OPENGL
		window::make_opengl_context_current();
#endif
#ifdef GRAPHICS_API_VULKAN
		if (!window::is_vulkan_supported()) {
			console::log_error("Vulkan is not supported on this system.");
			return false;
		}

		// PITFALL: On my laptop (ROG Zephyrus), there's a cross-reaction between these two
		// layers that causes vkEnumeratePhysicalDevices() to return 0 devices:
		// 
		// "VK_LAYER_NV_optimus"
		// "VK_LAYER_AMD_switchable_graphics"
		//
		// The first NVIDIA-provided layer filters out my integrated AMD GPU, while the second
		// AMD-provided layer filters out my dedicated NVIDIA GPU. I had to disable the second
		// layer to get the dedicated GPU to show up.
		//
		// https://stackoverflow.com/questions/68109171/vkenumeratephysicaldevices-not-finding-all-gpus

		platform::set_environment_variable("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");
#endif

		// INITIALIZE GRAPHICS API

		api::InitializeOptions options{};
#ifdef GRAPHICS_API_OPENGL
		options.glad_load_proc = window::get_glad_load_proc();
#endif
#ifdef GRAPHICS_API_VULKAN
		options.application_name = APPLICATION_NAME;
		options.engine_name = ENGINE_NAME;
		options.vulkan_instance_extensions = window::get_required_vulkan_instance_extensions();
#endif
#ifdef GRAPHICS_API_D3D11
		options.hwnd = window::get_win32_window();
#endif
		if (!api::initialize(options)) return false;

		// INITIALIZE SWAP CHAIN BACK BUFFER

		Framebuffer swap_chain_back_buffer{};
		swap_chain_back_buffer.api_handle = api::get_swap_chain_back_buffer();
		swap_chain_back_buffer.is_swap_chain_back_buffer = true;
		_swap_chain_back_buffer_handle = _framebuffer_pool.emplace(std::move(swap_chain_back_buffer));

		return true;
	}

	void shutdown() {

		// DELETE VERTEX SHADERS

		for (VertexShader& shader : _vertex_shader_pool.span()) {
			if (shader.api_handle.object) {
				api::destroy_vertex_shader(shader.api_handle);
				shader.api_handle = api::VertexShaderHandle();
			}
		}
		_vertex_shader_pool.clear();

		// DELETE FRAGMENT SHADERS

		for (FragmentShader& shader : _fragment_shader_pool.span()) {
			if (shader.api_handle.object) {
				api::destroy_fragment_shader(shader.api_handle);
				shader.api_handle = api::FragmentShaderHandle();
			}
		}
		_fragment_shader_pool.clear();

		// DELETE VERTEX INPUTS

		for (VertexInput& vertex_input : _vertex_input_pool.span()) {
			if (vertex_input.api_handle.object) {
				api::destroy_vertex_input(vertex_input.api_handle);
				vertex_input.api_handle = api::VertexInputHandle();
			}
		}
		_vertex_input_pool.clear();

		// DELETE BUFFERS

		for (Buffer& buffer : _buffer_pool.span()) {
			if (buffer.api_handle.object) {
				api::destroy_buffer(buffer.api_handle);
				buffer.api_handle = api::BufferHandle();
			}
		}
		_buffer_pool.clear();

		// DELETE TEXTURES

		for (Texture& texture : _texture_pool.span()) {
			if (texture.api_handle.object) {
				api::destroy_texture(texture.api_handle);
				texture.api_handle = api::TextureHandle();
			}
		}
		_texture_pool.clear();
		_path_to_texture.clear();

		// DELETE SAMPLERS

		for (Sampler& sampler : _sampler_pool.span()) {
			if (sampler.api_handle.object) {
				api::destroy_sampler(sampler.api_handle);
				sampler.api_handle = api::SamplerHandle();
			}
		}
		_sampler_pool.clear();

		// DELETE FRAMEBUFFERS

		for (Framebuffer& framebuffer : _framebuffer_pool.span()) {
			// The swap chain back buffer can't be destroyed the usual way
			// since it it handled differently from the user-created framebuffers.
			// For example, it might be owned by the window (if using OpenGL+GLFW),
			// or by the IDGXISwapChain (if using D3D11).
			if (framebuffer.is_swap_chain_back_buffer) continue;
			if (framebuffer.api_handle.object) {
				api::destroy_framebuffer(framebuffer.api_handle);
				framebuffer.api_handle = api::FramebufferHandle();
			}
		}
		_framebuffer_pool.clear();

		// DELETE RASTERIZER STATES

		for (RasterizerState& rasterizer_state : _rasterizer_state_pool.span()) {
			if (rasterizer_state.api_handle.object) {
				api::destroy_rasterizer_state(rasterizer_state.api_handle);
				rasterizer_state.api_handle = api::RasterizerStateHandle();
			}
		}
		_rasterizer_state_pool.clear();

		// DELETE BLEND STATES

		for (BlendState& blend_state : _blend_state_pool.span()) {
			if (blend_state.api_handle.object) {
				api::destroy_blend_state(blend_state.api_handle);
				blend_state.api_handle = api::BlendStateHandle();
			}
		}
		_blend_state_pool.clear();

		// SHUTDOWN API

		api::shutdown();
	}

	bool is_spirv_supported() {
		return api::is_spirv_supported();
	}

	bool resize_swap_chain_framebuffer(unsigned int new_width, unsigned int new_height) {
#ifdef GRAPHICS_API_OPENGL
		// The window owns the swap chain, so this is a no-op.
		return true;
#endif
#ifdef GRAPHICS_API_D3D11
		return api::resize_swap_chain_framebuffer(new_width, new_height);
#endif
	}

	unsigned int _swap_chain_sync_interval = 0;

	void set_swap_chain_sync_interval(unsigned int sync_interval) {
		_swap_chain_sync_interval = sync_interval;
#ifdef GRAPHICS_API_OPENGL
		// The window owns the swap chain.
		window::set_swap_chain_sync_interval(sync_interval);
#endif
	}

	void present_swap_chain_back_buffer() {
#ifdef GRAPHICS_API_OPENGL
		// The window owns the swap chain.
		window::present_swap_chain_back_buffer();
#endif
#ifdef GRAPHICS_API_D3D11
		api::present_swap_chain_back_buffer(_swap_chain_sync_interval);
#endif
	}

	void push_debug_group(std::string_view name) {
		api::push_debug_group(name);
	}

	void pop_debug_group() {
		api::pop_debug_group();
	}

	Handle<VertexShader> create_vertex_shader(ShaderDesc&& desc) {
		api::VertexShaderHandle api_handle = api::create_vertex_shader(desc);
		if (!api_handle.object) return Handle<VertexShader>();
		return _vertex_shader_pool.emplace(api_handle);
	}

	void bind_vertex_shader(Handle<VertexShader> handle) {
		if (handle == Handle<VertexShader>()) {
			api::bind_vertex_shader(api::VertexShaderHandle());
		} else if (const VertexShader* shader = _vertex_shader_pool.get(handle)) {
			api::bind_vertex_shader(shader->api_handle);
		}
	}

	Handle<FragmentShader> create_fragment_shader(ShaderDesc&& desc) {
		api::FragmentShaderHandle api_handle = api::create_fragment_shader(desc);
		if (!api_handle.object) return Handle<FragmentShader>();
		return _fragment_shader_pool.emplace(api_handle);
	}

	void bind_fragment_shader(Handle<FragmentShader> handle) {
		if (handle == Handle<FragmentShader>()) {
			api::bind_fragment_shader(api::FragmentShaderHandle());
		} else if (const FragmentShader* shader = _fragment_shader_pool.get(handle)) {
			api::bind_fragment_shader(shader->api_handle);
		}
	}

	Handle<VertexInput> create_vertex_input(VertexInputDesc&& desc) {
		api::VertexInputHandle api_handle = api::create_vertex_input(desc);
		if (!api_handle.object) return Handle<VertexInput>();
		return _vertex_input_pool.emplace(api_handle);
	}

	void bind_vertex_input(Handle<VertexInput> handle) {
		if (handle == Handle<VertexInput>()) {
			api::bind_vertex_input(api::VertexInputHandle());
		} else if (const VertexInput* vertex_input = _vertex_input_pool.get(handle)) {
			api::bind_vertex_input(vertex_input->api_handle);
		}
	}

	Handle<Buffer> create_buffer(BufferDesc&& desc) {
		api::BufferHandle api_handle = api::create_buffer(desc);
		if (!api_handle.object) return Handle<Buffer>();
		desc.initial_data = nullptr;
		return _buffer_pool.emplace(api_handle, std::move(desc));
	}

	void recreate_buffer(Handle<Buffer> handle, unsigned int size, const void* initial_data) {
		Buffer* buffer = _buffer_pool.get(handle);
		if (!buffer) return;
		api::destroy_buffer(buffer->api_handle);
		buffer->desc.size = size;
		buffer->desc.initial_data = initial_data;
		buffer->api_handle = api::create_buffer(buffer->desc);
		buffer->desc.initial_data = nullptr;
	}

	void destroy_buffer(Handle<Buffer> handle) {
		Buffer* buffer = _buffer_pool.get(handle);
		if (!buffer) return;
		api::destroy_buffer(buffer->api_handle);
		*buffer = Buffer();
		_buffer_pool.free(handle);
	}

	void update_buffer(Handle<Buffer> handle, const void* data, unsigned int size, unsigned int offset) {
		if (!data || !size) return;
		Buffer* buffer = _buffer_pool.get(handle);
		if (!buffer) return;
		if (!buffer->desc.dynamic) return;
		if (offset + size > buffer->desc.size) return;
		api::update_buffer(buffer->api_handle, data, size, offset);
	}

	void update_or_recreate_buffer(Handle<Buffer> handle, const void* data, unsigned int size) {
		if (!data || !size) return;
		Buffer* buffer = _buffer_pool.get(handle);
		if (!buffer) return;
		if (!buffer->desc.dynamic) return;
		if (size <= buffer->desc.size) {
			api::update_buffer(buffer->api_handle, data, size, 0);
			return;
		}
		api::destroy_buffer(buffer->api_handle);
		buffer->desc.size = size;
		buffer->desc.initial_data = data;
		buffer->api_handle = api::create_buffer(buffer->desc);
		buffer->desc.initial_data = nullptr;
	}

	size_t get_buffer_size(Handle<Buffer> handle) {
		if (const Buffer* buffer = _buffer_pool.get(handle)) {
			return buffer->desc.size;
		}
		return 0;
	}

	void bind_vertex_buffer(unsigned int binding, Handle<Buffer> handle, unsigned int stride, unsigned int offset) {
		if (handle == Handle<Buffer>()) {
			api::bind_vertex_buffer(binding, api::BufferHandle(), 0, 0);
		} else if (const Buffer* buffer = _buffer_pool.get(handle)) {
			api::bind_vertex_buffer(binding, buffer->api_handle, stride, offset);
		}
	}

	void bind_index_buffer(Handle<Buffer> handle) {
		if (handle == Handle<Buffer>()) {
			api::bind_index_buffer(api::BufferHandle());
		} else if (const Buffer* buffer = _buffer_pool.get(handle)) {
			api::bind_index_buffer(buffer->api_handle);
		}
	}

	void bind_uniform_buffer(unsigned int binding, Handle<Buffer> handle) {
		if (handle == Handle<Buffer>()) {
			api::bind_uniform_buffer(binding, api::BufferHandle());
		} else if (const Buffer* buffer = _buffer_pool.get(handle)) {
			api::bind_uniform_buffer(binding, buffer->api_handle);
		}
	}

	void bind_uniform_buffer_range(unsigned int binding, Handle<Buffer> handle, unsigned int size, unsigned offset) {
		const Buffer* buffer = _buffer_pool.get(handle);
		if (!buffer) return;
		if (offset + size > buffer->desc.size) return;
		api::bind_uniform_buffer_range(binding, buffer->api_handle, size, offset);
	}

	unsigned int _format_to_channels(Format format) {
		switch (format) {
		case Format::R8_UNORM:    return 1;
		case Format::RG8_UNORM:   return 2;
		case Format::RGB8_UNORM:  return 3;
		case Format::RGBA8_UNORM: return 4;
		default: return 0;
		}
	}

	unsigned int _get_texture_byte_size(const TextureDesc& desc) {
		return desc.width * desc.height * _format_to_channels(desc.format);
	}

	Handle<Texture> create_texture(TextureDesc&& desc) {
		api::TextureHandle api_handle = api::create_texture(desc);
		if (!api_handle.object) return Handle<Texture>();
		desc.initial_data = nullptr;
		_total_texture_memory_usage_in_bytes += _get_texture_byte_size(desc);
		return _texture_pool.emplace(api_handle, std::move(desc));
	}

	Format _channels_to_format(unsigned int channels) {
		switch (channels) {
		case 1:  return Format::R8_UNORM;
		case 2:  return Format::RG8_UNORM;
		case 3:  return Format::RGB8_UNORM;
		case 4:  return Format::RGBA8_UNORM;
		default: return Format::UNKNOWN;
		}
	}

	Handle<Texture> load_texture(std::string_view path) {

		std::string normalized_path = files::get_normalized_path(path);
		// KTX2 compressed textures load much MUCH faster, so we prefer those whenever possible.
		std::string normalized_path_ktx2 = files::replace_extension(normalized_path, ".ktx2");

		// Check if the KTX2 texture is already loaded.
		if (const auto it = _path_to_texture.find(normalized_path_ktx2); it != _path_to_texture.end()) {
			return it->second;
		}
		// Check if the non-KTX2 texture is already loaded.
		if (const auto it = _path_to_texture.find(normalized_path); it != _path_to_texture.end()) {
			return it->second;
		}

		images::Image image{};
		std::string path_used;
		
		if (files::file_exists(normalized_path_ktx2)) {
			// Try to load a KTX2 texture first if it exists.
			if (!images::load_image(normalized_path_ktx2, image)) {
				return Handle<Texture>();
			}
			path_used = std::move(normalized_path_ktx2);
		} else if (files::file_exists(normalized_path)) {
			// Fall back to loading the non-KTX2 texture.
			if (!images::load_image(normalized_path, image)) {
				return Handle<Texture>();
			}
			path_used = std::move(normalized_path);
		} else {
			console::log_error("Failed to load texture: " + normalized_path);
			return Handle<Texture>();
		}

		const Format format = _channels_to_format(image.channels);
		if (format == Format::UNKNOWN) {
			console::log_error("Unsupported texture channel count:");
			console::log_error("- Texture: " + std::string(path_used));
			console::log_error("- Channels: " + std::to_string(image.channels));
			images::free_image(image); // Don't forget!
			return Handle<Texture>();
		}

		const Handle<Texture> handle = create_texture({
			// PITFALL: Since path_used goes out of scope once this function returns,
			// setting debug_name to it would usually lead to undefined behavior.
			// I've made it so that we move path_used into the unordered map,
			// which takes ownership of the string and thus keeps it alive.
			// Admittedly, this is a bit of a hack, but it seems to work.
			.debug_name = path_used,
			.width = image.width,
			.height = image.height,
			.format = format,
			.initial_data = image.data
		});

		images::free_image(image); // Don't forget!

		// CRITICAL: Move path_used so it is kept alive.
		_path_to_texture[std::move(path_used)] = handle;

		return handle;
	}

	Handle<Texture> copy_texture(Handle<Texture> src) {
		Handle<Texture> dest;
		if (const Texture* src_texture = _texture_pool.get(src)) {
			dest = create_texture(TextureDesc(src_texture->desc));
			copy_texture(dest, src);
		}
		return dest;
	}

	void destroy_texture(Handle<Texture> handle) {
		Texture* texture = _texture_pool.get(handle);
		if (!texture) return;
		api::destroy_texture(texture->api_handle);
		_total_texture_memory_usage_in_bytes -= _get_texture_byte_size(texture->desc);
		// HACK: When a texture is loaded, its debug_name is set to the path.
		_path_to_texture.erase(std::string(texture->desc.debug_name));
		*texture = Texture();
		_texture_pool.free(handle);
	}

	void bind_texture(unsigned int binding, Handle<Texture> handle) {
		if (handle == Handle<Texture>()) {
			api::bind_texture(binding, api::TextureHandle());
		} else if (const Texture* texture = _texture_pool.get(handle)) {
			api::bind_texture(binding, texture->api_handle);
		}
	}

	void update_texture(Handle<Texture> handle, const unsigned char* data) {
		const Texture* texture = _texture_pool.get(handle);
		if (!texture) return;
		api::update_texture(texture->api_handle, 0, 0, 0,
			texture->desc.width, texture->desc.height, texture->desc.format, data);
	}

	void copy_texture(Handle<Texture> dest, Handle<Texture> src) {
		Texture* dest_texture = _texture_pool.get(dest);
		const Texture* src_texture = _texture_pool.get(src);
		if (!dest_texture || !src_texture) return;
		if (dest_texture->desc.width != src_texture->desc.width) return;
		if (dest_texture->desc.height != src_texture->desc.height) return;
		api::copy_texture(
			dest_texture->api_handle, 0, 0, 0, 0,
			src_texture->api_handle, 0, 0, 0, 0,
			src_texture->desc.width, src_texture->desc.height, 1);
	}

	Vec2u get_texture_size(Handle<Texture> handle) {
		Vec2u size;
		if (const Texture* texture = _texture_pool.get(handle)) {
			size.x = texture->desc.width;
			size.y = texture->desc.height;
		}
		return size;
	}

	Handle<Sampler> create_sampler(SamplerDesc&& desc) {
		api::SamplerHandle api_handle = api::create_sampler(desc);
		if (!api_handle.object) return Handle<Sampler>();
		return _sampler_pool.emplace(api_handle, std::move(desc));
	}

	void destroy_sampler(Handle<Sampler> handle) {
		Sampler* sampler = _sampler_pool.get(handle);
		if (!sampler) return;
		api::destroy_sampler(sampler->api_handle);
		*sampler = Sampler();
		_sampler_pool.free(handle);
	}

	void bind_sampler(unsigned int binding, Handle<Sampler> handle) {
		if (handle == Handle<Sampler>()) {
			api::bind_sampler(binding, api::SamplerHandle());
		} else if (const Sampler* sampler = _sampler_pool.get(handle)) {
			api::bind_sampler(binding, sampler->api_handle);
		}
	}

	Handle<Framebuffer> get_swap_chain_back_buffer() {
		return _swap_chain_back_buffer_handle;
	}

	Handle<Framebuffer> create_framebuffer(FramebufferDesc&& desc) {
		api::FramebufferHandle api_handle = api::create_framebuffer(desc);
		if (!api_handle.object) return Handle<Framebuffer>();
		return _framebuffer_pool.emplace(api_handle, std::move(desc));
	}

	void attach_framebuffer_texture(Handle<Framebuffer> framebuffer_handle, Handle<Texture> texture_handle) {
		Framebuffer* framebuffer = _framebuffer_pool.get(framebuffer_handle);
		if (!framebuffer) return;
		if (framebuffer->is_swap_chain_back_buffer) {
			console::log_error("Cannot attach a texture to the swap chain back buffer.");
			return;
		}
		Texture* texture = _texture_pool.get(texture_handle);
		if (!texture) return;
		if (!api::attach_framebuffer_color_texture(framebuffer->api_handle, 0, texture->api_handle)) {
			console::log_error(
				"Failed to attach texture " +
				std::string(texture->desc.debug_name) +
				" to framebuffer " +
				std::string(framebuffer->desc.debug_name));
			return;
		}
		framebuffer->texture = texture_handle;
	}

	Handle<Texture> get_framebuffer_texture(Handle<Framebuffer> handle) {
		const Framebuffer* framebuffer = _framebuffer_pool.get(handle);
		if (!framebuffer) return Handle<Texture>();
		if (framebuffer->is_swap_chain_back_buffer) {
			console::log_error("Cannot get the texture of the swap chain back buffer.");
			return Handle<Texture>();
		}
		return framebuffer->texture;
	}

	void resize_framebuffer(Handle<Framebuffer> framebuffer_handle, unsigned int width, unsigned int height) {
		if (width == 0 || height == 0) return; // Illegal size
		Framebuffer* framebuffer = _framebuffer_pool.get(framebuffer_handle);
		if (!framebuffer) return;
		if (framebuffer->is_swap_chain_back_buffer) {
			console::log_error("Cannot resize the swap chain back buffer.");
			return;
		}
		Texture* texture = _texture_pool.get(framebuffer->texture);
		if (!texture) return;
		if (texture->desc.width == width && texture->desc.height == height) return; // No-op
		api::destroy_texture(texture->api_handle);
		_total_texture_memory_usage_in_bytes -= _get_texture_byte_size(texture->desc);
		texture->desc.width = width;
		texture->desc.height = height;
		texture->api_handle = api::create_texture(texture->desc);
		_total_texture_memory_usage_in_bytes += _get_texture_byte_size(texture->desc);
		api::attach_framebuffer_color_texture(framebuffer->api_handle, 0, texture->api_handle);
	}

	void bind_framebuffer(Handle<Framebuffer> handle) {
		if (handle == Handle<Framebuffer>()) {
			api::bind_framebuffer(api::FramebufferHandle());
		} else if (const Framebuffer* framebuffer = _framebuffer_pool.get(handle)) {
			api::bind_framebuffer(framebuffer->api_handle);
		}
	}

	void clear_framebuffer(Handle<Framebuffer> handle, const float color[4]) {
		if (const Framebuffer* framebuffer = _framebuffer_pool.get(handle)) {
			api::clear_framebuffer_color(framebuffer->api_handle, 0, color);
		}
	}

	Handle<RasterizerState> create_rasterizer_state(RasterizerDesc&& desc) {
		api::RasterizerStateHandle api_handle = api::create_rasterizer_state(desc);
		if (!api_handle.object) return Handle<RasterizerState>();
		return _rasterizer_state_pool.emplace(api_handle, std::move(desc));
	}

	void bind_rasterizer_state(Handle<RasterizerState> handle) {
		if (handle == Handle<RasterizerState>()) {
			api::bind_rasterizer_state(api::RasterizerStateHandle());
		} else if (const RasterizerState* state = _rasterizer_state_pool.get(handle)) {
			api::bind_rasterizer_state(state->api_handle);
		}
	}

	Handle<BlendState> create_blend_state(BlendDesc&& desc) {
		api::BlendStateHandle api_handle = api::create_blend_state(desc);
		if (!api_handle.object) return Handle<BlendState>();
		return _blend_state_pool.emplace(api_handle, std::move(desc));
	}

	void bind_blend_state(Handle<BlendState> handle) {
		if (handle == Handle<BlendState>()) {
			api::bind_blend_state(api::BlendStateHandle());
		} else if (const BlendState* state = _blend_state_pool.get(handle)) {
			api::bind_blend_state(state->api_handle);
		}
	}

	void set_primitives(Primitives primitives) {
		api::set_primitives(primitives);
	}

	void set_viewport(const Viewport& viewport) {
		api::set_viewports(&viewport, 1);
		_viewport = viewport;
	}

	void get_viewport(Viewport& viewport) {
		viewport = _viewport;
	}

	void set_scissor(const Rect& scissor) {
		api::set_scissors(&scissor, 1);
		_scissor = scissor;
	}

	void set_scissor_test_enabled(bool enable) {
		api::set_scissor_test_enabled(enable);
		_scissor_test_enabled = enable;
	}

	bool get_scissor_test_enabled() {
		return _scissor_test_enabled;
	}

	void get_scissor(Rect& scissor) {
		scissor = _scissor;
	}

	void draw(unsigned int vertex_count, unsigned int vertex_offset) {
		api::draw(vertex_count, vertex_offset);
	}

	void draw_indexed(unsigned int index_count) {
		api::draw_indexed(index_count);
	}

	void show_texture_debug_window() {
		ImGui::Begin("Textures");
		ImGui::Text("Total memory usage: %d MB", _total_texture_memory_usage_in_bytes / 1024 / 1024);
		for (size_t i = 0; i < _texture_pool.size(); ++i) {
			const Texture& texture = _texture_pool.data()[i];
			if (!texture.api_handle.object) continue;
			if (ImGui::TreeNode(texture.desc.debug_name.data())) {
				ImGui::Text("Dimensions: %dx%dx", texture.desc.width, texture.desc.height);
				ImGui::Text("Format: %s", magic_enum::enum_name(texture.desc.format).data());
				unsigned int size = _get_texture_byte_size(texture.desc);
				unsigned int kb = size / 1024;
				unsigned int mb = kb / 1024;
				if (mb) {
					ImGui::Text("Memory: %d MB", mb);
				} else {
					ImGui::Text("Memory: %d KB", kb);
				}
				ImVec2 texture_size = ImVec2((float)texture.desc.width, (float)texture.desc.height);
				ImGui::Image((ImTextureID)texture.api_handle.object, texture_size);
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}
}