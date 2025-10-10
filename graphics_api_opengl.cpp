#include "graphics_api.h"
#ifdef GRAPHICS_API_OPENGL
#include <glad/glad.h>
#include <string>
#include <vector>

#pragma comment(lib, "opengl32")

#define MAX_VIEWPORTS 8u

// Undefine pre-DSA functions to force the use of DSA whenever possible.

#undef glGenTextures
#undef glBindTexture
#undef glTexParameterf
#undef glTexParameteri
#undef glTexParameterfv
#undef glTexParameteriv
#undef glTexParameterIiv
#undef glTexParameterIuiv
#undef glTexImage2D
#undef glTexStorage2D
#undef glTexSubImage2D
#undef glGenerateMipmap
#undef glActiveTexture

#undef glGenFramebuffers
#undef glFramebufferTexture2D 
#undef glCheckFramebufferStatus
#undef glBlitFramebuffer
#undef glClearColor
#undef glClearDepth
#undef glClearStencil
#undef glClear
#undef glClearBufferiv
#undef glClearBufferuiv
#undef glClearBufferfv
#undef glClearBufferfi

#undef glGenBuffer
#undef glBufferData
#undef glBufferSubData
#undef glBufferStorage
#undef glNamedBufferData

#undef glGenVertexArrays
#undef glEnableVertexAttribArray
#undef glDisableVertexAttribArray
#undef glVertexAttribPointer
#undef glVertexAttribFormat
#undef glVertexAttribIFormat
#undef glVertexAttribLFormat
#undef glVertexAttribBinding
#undef glVertexBindingDivisor
#undef glBindVertexBuffer

#undef glViewport
#undef glViewportIndexedf
#undef glViewportIndexedfv
#undef glDepthRange
#undef glDepthRangef
#undef glDepthRangeIndexed
#undef glScissor
#undef glScissorIndexed
#undef glScissorIndexedv

typedef unsigned long DWORD;

extern "C" {
	//This is a hack to make sure that OpenGL chooses the dedicated GPU on laptops
	//with switchable graphics. It works by exporting a variable that the driver
	//looks for when initializing the OpenGL context.
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; // NVIDIA: Force High-Performance GPU
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; // AMD: Force High-Performance GPU
}

namespace graphics {
namespace api {

	DebugMessageCallback _debug_message_callback = nullptr;

	void set_debug_message_callback(DebugMessageCallback callback) {
		_debug_message_callback = callback;
	}

	void _output_debug_message(std::string_view message) {
		if (_debug_message_callback) {
			_debug_message_callback(message);
		}
	}

#ifdef GRAPHICS_API_DEBUG
	void GLAPIENTRY _gl_debug_message_callback(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam
	) {
		_output_debug_message(message);
	}
#endif

	GLuint _program_pipeline_object = 0;
	bool _is_spirv_supported = false;

	void _gl_object_label(GLenum identifier, GLuint name, std::string_view label) {
#ifdef GRAPHICS_API_DEBUG
		if (label.empty()) return;
		glObjectLabel(identifier, name, (GLsizei)label.size(), label.data());
#endif
	}

	bool initialize(const InitializeOptions& options) {

		// INITIALIZE GLAD

		if (!gladLoadGLLoader((GLADloadproc)options.glad_load_proc)) {
			_output_debug_message("Failed to initialize GLAD");
			return false;
		}

		// ENABLE DEBUG OUTPUT

#ifdef GRAPHICS_API_DEBUG
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(_gl_debug_message_callback, 0);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_MARKER, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PUSH_GROUP, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_POP_GROUP, GL_DONT_CARE, 0, nullptr, GL_FALSE);
#endif

		// CHECK GRAPHICS CARD
		{
			const char* vendor = (const char*)glGetString(GL_VENDOR);
			const char* renderer = (const char*)glGetString(GL_RENDERER);
			const char* version = (const char*)glGetString(GL_VERSION);
		}

		// CHECK SPIR-V SUPPORT
		{
			GLint binary_format_count = 0;
			glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &binary_format_count);
			std::vector<GLint> binary_formats(binary_format_count);
			glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, binary_formats.data());
			for (GLint binary_format : binary_formats) {
				if (binary_format == GL_SHADER_BINARY_FORMAT_SPIR_V) {
					_is_spirv_supported = true;
					break;
				}
			}
		}

		// SETUP PRIMITIVE INDEX RESTART

		glEnable(GL_PRIMITIVE_RESTART);
		glPrimitiveRestartIndex(UINT32_MAX);

		// SETUP PROGRAM PIPELINE OBJECT

		glCreateProgramPipelines(1, &_program_pipeline_object);
		glBindProgramPipeline(_program_pipeline_object);
		_gl_object_label(GL_PROGRAM_PIPELINE, _program_pipeline_object, "program pipeline");

		return true;
	}

	void shutdown() {
		if (_program_pipeline_object) {
			glDeleteProgramPipelines(1, &_program_pipeline_object);
		}
	}

	bool is_spirv_supported() {
		return _is_spirv_supported;
	}

	void push_debug_group(std::string_view name) {
#ifdef GRAPHICS_API_DEBUG
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, (GLsizei)name.size(), name.data());
#endif
	}

	void pop_debug_group() {
#ifdef GRAPHICS_API_DEBUG
		glPopDebugGroup();
#endif
	}

	GLuint _create_shader_program(const ShaderDesc& desc, GLenum shader_type) {
		if (desc.code.empty()) {
			_output_debug_message("Shader code is empty: " + std::string(desc.debug_name));
			return 0;
		}
		const GLuint shader_object = glCreateShader(shader_type);
		if (desc.binary) { // SPIR-V
			if (!_is_spirv_supported) {
				_output_debug_message("SPIR-V is not supported on this system: " + std::string(desc.debug_name));
				glDeleteShader(shader_object);
				return 0;
			}
			glShaderBinary(1, &shader_object, GL_SHADER_BINARY_FORMAT_SPIR_V, desc.code.data(), (GLsizei)desc.code.size());
			glSpecializeShader(shader_object, "main", 0, nullptr, nullptr);
		} else { // GLSL
			const char* source_code_string = (const char*)desc.code.data();
			const GLint source_code_length = (GLint)desc.code.size();
			glShaderSource(shader_object, 1, &source_code_string, &source_code_length);
			glCompileShader(shader_object);
		}
		GLint success = GL_FALSE;
		glGetShaderiv(shader_object, GL_COMPILE_STATUS, &success);
		if (!success) {
#ifdef GRAPHICS_API_DEBUG
			char info_log[512];
			glGetShaderInfoLog(shader_object, sizeof(info_log), nullptr, info_log);
			_output_debug_message("Failed to compile shader: " + std::string(desc.debug_name));
			_output_debug_message(info_log);
#endif
			glDeleteShader(shader_object);
			return 0;
		}
		const GLuint program_object = glCreateProgram();
		glProgramParameteri(program_object, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glAttachShader(program_object, shader_object);
		glLinkProgram(program_object);
		glDetachShader(program_object, shader_object);
		glDeleteShader(shader_object);
		glGetProgramiv(program_object, GL_LINK_STATUS, &success);
		if (!success) {
#ifdef GRAPHICS_API_DEBUG
			char info_log[512];
			glGetProgramInfoLog(program_object, sizeof(info_log), nullptr, info_log);
			_output_debug_message("Failed to link program: " + std::string(desc.debug_name));
			_output_debug_message(info_log);
#endif
			glDeleteProgram(program_object);
			return 0;
		}
		_gl_object_label(GL_PROGRAM, program_object, desc.debug_name);
		return program_object;
	}

	VertexShaderHandle create_vertex_shader(const ShaderDesc& desc) {
		return VertexShaderHandle{ _create_shader_program(desc, GL_VERTEX_SHADER) };
	}

	void destroy_vertex_shader(VertexShaderHandle shader) {
		glDeleteProgram((GLuint)shader.object);
	}

	void bind_vertex_shader(VertexShaderHandle shader) {
		glUseProgramStages(_program_pipeline_object, GL_VERTEX_SHADER_BIT, (GLuint)shader.object);
	}

	FragmentShaderHandle create_fragment_shader(const ShaderDesc& desc) {
		return FragmentShaderHandle{ _create_shader_program(desc, GL_FRAGMENT_SHADER) };
	}

	void destroy_fragment_shader(FragmentShaderHandle shader) {
		glDeleteProgram((GLuint)shader.object);
	}

	void bind_fragment_shader(FragmentShaderHandle shader) {
		glUseProgramStages(_program_pipeline_object, GL_FRAGMENT_SHADER_BIT, (GLuint)shader.object);
	}

	GLuint _get_vertex_attribute_component_count(Format format) {
		// must be 1, 2, 3, 4, or GL_BGRA
		switch (format) {
		case Format::R8_UNORM:     return 1;
		case Format::RG8_UNORM:    return 2;
		case Format::RGB8_UNORM:   return 3;
		case Format::RGBA8_UNORM:  return 4;
		case Format::R32_FLOAT:    return 1;
		case Format::RG32_FLOAT:   return 2;
		case Format::RGB32_FLOAT:  return 3;
		case Format::RGBA32_FLOAT: return 4;
		default: return 0;
		}
	}

	GLenum _get_vertex_attribute_component_type(Format format) {
		switch (format) {
		case Format::R8_UNORM:     return GL_UNSIGNED_BYTE;
		case Format::RG8_UNORM:    return GL_UNSIGNED_BYTE;
		case Format::RGB8_UNORM:   return GL_UNSIGNED_BYTE;
		case Format::RGBA8_UNORM:  return GL_UNSIGNED_BYTE;
		case Format::R32_FLOAT:    return GL_FLOAT;
		case Format::RG32_FLOAT:   return GL_FLOAT;
		case Format::RGB32_FLOAT:  return GL_FLOAT;
		case Format::RGBA32_FLOAT: return GL_FLOAT;
		default: return 0;
		}
	}

	VertexInputHandle create_vertex_input(const VertexInputDesc& desc) {
		GLuint vertex_array_object = 0;
		glCreateVertexArrays(1, &vertex_array_object);
		_gl_object_label(GL_VERTEX_ARRAY, vertex_array_object, desc.debug_name);
		glBindVertexArray(vertex_array_object);
		for (unsigned int i = 0; i < desc.attributes.size(); ++i) {
			const VertexInputAttribDesc& attrib = *(desc.attributes.begin() + i);
			glEnableVertexArrayAttrib(vertex_array_object, i);
			glVertexArrayAttribFormat(vertex_array_object, i,
				_get_vertex_attribute_component_count(attrib.format),
				_get_vertex_attribute_component_type(attrib.format),
				attrib.normalized ? GL_TRUE : GL_FALSE,
				attrib.offset);
			glVertexArrayAttribBinding(vertex_array_object, i, attrib.binding);
		}
		return VertexInputHandle{ vertex_array_object };
	}

	void destroy_vertex_input(VertexInputHandle sprite_vertex_input) {
		glDeleteVertexArrays(1, (GLuint*)&sprite_vertex_input.object);
	}

	void bind_vertex_input(VertexInputHandle sprite_vertex_input) {
		glBindVertexArray((GLuint)sprite_vertex_input.object);
	}

	BufferHandle create_buffer(const BufferDesc& desc) {
		GLuint buffer_object = 0;
		glCreateBuffers(1, &buffer_object);
		_gl_object_label(GL_BUFFER, buffer_object, desc.debug_name);
		GLbitfield flags = 0;
		if (desc.dynamic) {
			flags |= GL_DYNAMIC_STORAGE_BIT;
		}
		glNamedBufferStorage(buffer_object, desc.size, desc.initial_data, flags);
		return BufferHandle{ buffer_object };
	}

	void destroy_buffer(BufferHandle buffer) {
		glDeleteBuffers(1, (GLuint*)&buffer);
	}

	void update_buffer(BufferHandle buffer, const void* data, unsigned int size, unsigned int offset) {
		glNamedBufferSubData((GLuint)buffer.object, offset, size, data);
	}

	void bind_uniform_buffer(unsigned int binding, BufferHandle buffer) {
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, (GLuint)buffer.object);
	}

	void bind_uniform_buffer_range(unsigned int binding, BufferHandle buffer, unsigned int size, unsigned int offset) {
		glBindBufferRange(GL_UNIFORM_BUFFER, binding, (GLuint)buffer.object, offset, size);
	}

	void bind_vertex_buffer(unsigned int binding, BufferHandle buffer, unsigned int stride, unsigned int offset) {
		GLint vertex_array_object = 0;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertex_array_object);
		if (vertex_array_object == 0) {
			_output_debug_message("No vertex array object is bound");
			return;
		}
		glVertexArrayVertexBuffer(vertex_array_object, binding, (GLuint)buffer.object, offset, stride);
	}

	void bind_index_buffer(BufferHandle buffer, unsigned int /*offset*/) {
		GLint vertex_array_object = 0;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertex_array_object);
		if (vertex_array_object == 0) {
			_output_debug_message("No vertex array object is bound");
			return;
		}
		glVertexArrayElementBuffer(vertex_array_object, (GLuint)buffer.object);
	}

	GLenum _to_gl_base_format(Format format) {
		switch (format) {
		case Format::R8_UNORM:    return GL_RED;
		case Format::RG8_UNORM:   return GL_RG;
		case Format::RGB8_UNORM:  return GL_RGB;
		case Format::RGBA8_UNORM: return GL_RGBA;
		default: return 0;
		}
	}

	GLenum _to_gl_sized_format(Format format) {
		switch (format) {
		case Format::R8_UNORM:    return GL_R8;
		case Format::RG8_UNORM:   return GL_RG8;
		case Format::RGB8_UNORM:  return GL_RGB8;
		case Format::RGBA8_UNORM: return GL_RGBA8;
		default: return 0;
		}
	}

	TextureHandle create_texture(const TextureDesc& desc) {
		GLuint texture_object = 0;
		glCreateTextures(GL_TEXTURE_2D, 1, &texture_object);
		_gl_object_label(GL_TEXTURE, texture_object, desc.debug_name);
		glTextureStorage2D(texture_object, 1, _to_gl_sized_format(desc.format), desc.width, desc.height);
		if (desc.initial_data) {
			glTextureSubImage2D(
				texture_object,
				0, // level
				0, // xoffset
				0, // yoffset
				desc.width,
				desc.height,
				_to_gl_base_format(desc.format),
				GL_UNSIGNED_BYTE,
				desc.initial_data
			);
		}
		return TextureHandle{ texture_object };
	}

	void destroy_texture(TextureHandle texture) {
		glDeleteTextures(1, (GLuint*)&texture.object);
	}

	void update_texture(
		TextureHandle texture,
		unsigned int level,
		unsigned int x,
		unsigned int y,
		unsigned int width,
		unsigned int height,
		Format pixel_format,
		const void* pixels
	) {
		glTextureSubImage2D(
			(GLuint)texture.object,
			level,
			x,
			y,
			width,
			height,
			_to_gl_base_format(pixel_format),
			GL_UNSIGNED_BYTE,
			pixels
		);
	}

	void copy_texture(
		TextureHandle dst_texture,
		unsigned int dst_level,
		unsigned int dst_x,
		unsigned int dst_y,
		unsigned int dst_z,
		TextureHandle src_texture,
		unsigned int src_level,
		unsigned int src_x,
		unsigned int src_y,
		unsigned int src_z,
		unsigned int src_width,
		unsigned int src_height,
		unsigned int src_depth
	) {
		glCopyImageSubData(
			(GLuint)src_texture.object,
			GL_TEXTURE_2D,
			src_level,
			src_x,
			src_y,
			src_z,
			(GLuint)dst_texture.object,
			GL_TEXTURE_2D,
			dst_level,
			dst_x,
			dst_y,
			dst_z,
			src_width,
			src_height,
			src_depth
		);
	}

	void bind_texture(unsigned int binding, TextureHandle texture) {
		glBindTextureUnit(binding, (GLuint)texture.object);
	}

	GLint _to_gl_filter(Filter filter) {
		switch (filter) {
		case Filter::Nearest: return GL_NEAREST;
		case Filter::Linear:  return GL_LINEAR;
		default:			  return 0;
		}
	}

	GLint _to_gl_wrap(Wrap wrap) {
		switch (wrap) {
		case Wrap::Repeat:            return GL_REPEAT;
		case Wrap::MirroredRepeat:    return GL_MIRRORED_REPEAT;
		case Wrap::ClampToEdge:       return GL_CLAMP_TO_EDGE;
		case Wrap::ClampToBorder:     return GL_CLAMP_TO_BORDER;
		case Wrap::MirrorClampToEdge: return GL_MIRROR_CLAMP_TO_EDGE;
		default:				      return 0;
		}
	}

	SamplerHandle create_sampler(const SamplerDesc& desc) {
		GLuint sampler_object = 0;
		glCreateSamplers(1, &sampler_object);
		_gl_object_label(GL_SAMPLER, sampler_object, desc.debug_name);
		const GLint gl_filter = _to_gl_filter(desc.filter);
		glSamplerParameteri(sampler_object, GL_TEXTURE_MIN_FILTER, gl_filter);
		glSamplerParameteri(sampler_object, GL_TEXTURE_MAG_FILTER, gl_filter);
		const GLint gl_wrap = _to_gl_wrap(desc.wrap);
		glSamplerParameteri(sampler_object, GL_TEXTURE_WRAP_S, gl_wrap);
		glSamplerParameteri(sampler_object, GL_TEXTURE_WRAP_T, gl_wrap);
		glSamplerParameterfv(sampler_object, GL_TEXTURE_BORDER_COLOR, desc.border_color);
		return SamplerHandle{ sampler_object };
	}

	void destroy_sampler(SamplerHandle sampler) {
		glDeleteSamplers(1, (GLuint*)&sampler.object);
	}

	void bind_sampler(unsigned int binding, SamplerHandle sampler) {
		glBindSampler(binding, (GLuint)sampler.object);
	}

	FramebufferHandle get_swap_chain_back_buffer() {
		return FramebufferHandle{ 0 };
	}

	FramebufferHandle create_framebuffer(const FramebufferDesc& desc) {
		GLuint framebuffer_object = 0;
		glCreateFramebuffers(1, &framebuffer_object);
		_gl_object_label(GL_FRAMEBUFFER, framebuffer_object, desc.debug_name);
		return FramebufferHandle{ framebuffer_object };
	}

	void destroy_framebuffer(FramebufferHandle framebuffer_color) {
		glDeleteFramebuffers(1, (GLuint*)&framebuffer_color.object);
	}

	bool attach_framebuffer_color_texture(FramebufferHandle framebuffer_color, unsigned int attachment, TextureHandle texture) {
		glNamedFramebufferTexture((GLuint)framebuffer_color.object, GL_COLOR_ATTACHMENT0 + attachment, (GLuint)texture.object, 0);
		return glCheckNamedFramebufferStatus((GLuint)framebuffer_color.object, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}

	void clear_framebuffer_color(FramebufferHandle framebuffer_color, unsigned int attachment, const float color[4]) {
		glClearNamedFramebufferfv((GLuint)framebuffer_color.object, GL_COLOR, attachment, color);
	}

	void bind_framebuffer(FramebufferHandle framebuffer_color) {
		glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)framebuffer_color.object);
	}

	RasterizerStateHandle create_rasterizer_state(const RasterizerDesc& desc) {
		RasterizerDesc* impl = new RasterizerDesc(desc);
		impl->debug_name = {};
		return RasterizerStateHandle{ .object = (uintptr_t)impl };
	}

	void destroy_rasterizer_state(RasterizerStateHandle state) {
		if (!state.object) return;
		RasterizerDesc* impl = (RasterizerDesc*)state.object;
		delete impl;
	}

	GLenum _polygon_mode_to_gl_polygon_mode(PolygonMode mode) {
		switch (mode) {
		case PolygonMode::Fill: return GL_FILL;
		case PolygonMode::Line: return GL_LINE;
		default:                return 0;
		}
	}

	void bind_rasterizer_state(RasterizerStateHandle state) {
		if (!state.object) return; // TODO: default state
		RasterizerDesc* impl = (RasterizerDesc*)state.object;
		glPolygonMode(GL_FRONT_AND_BACK, _polygon_mode_to_gl_polygon_mode(impl->polygon_mode));
		if (impl->cull_mode == CullMode::None) {
			glDisable(GL_CULL_FACE);
		} else {
			glEnable(GL_CULL_FACE);
			glCullFace(impl->cull_mode == CullMode::Front ? GL_FRONT : GL_BACK);
		}
		glFrontFace(impl->front_face_ccw ? GL_CCW : GL_CW);
	}

	GLenum _blend_factor_to_gl_blend_factor(BlendFactor factor) {
		switch (factor) {
		case BlendFactor::Zero:             return GL_ZERO;
		case BlendFactor::One:              return GL_ONE;
		case BlendFactor::SrcColor:         return GL_SRC_COLOR;
		case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case BlendFactor::DstColor:         return GL_DST_COLOR;
		case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlpha:         return GL_SRC_ALPHA;
		case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha:         return GL_DST_ALPHA;
		case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
		default:                            return GL_ZERO;
		}
	}

	GLenum _blend_op_to_gl_blend_op(BlendOp op) {
		switch (op) {
		case BlendOp::Add:             return GL_FUNC_ADD;
		case BlendOp::Subtract:        return GL_FUNC_SUBTRACT;
		case BlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
		case BlendOp::Min:             return GL_MIN;
		case BlendOp::Max:             return GL_MAX;
		default:                       return GL_FUNC_ADD;
		}
	}

	BlendStateHandle create_blend_state(const BlendDesc& desc) {
		BlendDesc* impl = new BlendDesc(desc);
		impl->debug_name = {};
		return BlendStateHandle{ .object = (uintptr_t)impl };
	}

	void destroy_blend_state(BlendStateHandle state) {
		if (!state.object) return;
		BlendDesc* impl = (BlendDesc*)state.object;
		delete impl;
	}

	void bind_blend_state(BlendStateHandle state) {
		if (!state.object) return; // TODO: default state
		BlendDesc* impl = (BlendDesc*)state.object;
		for (GLuint i = 0; i < impl->attachments.size(); ++i) {
			const AttachmentBlendDesc& attachment_blend_desc = *(impl->attachments.begin() + i);
			if (attachment_blend_desc.blend_enable) {
				glEnablei(GL_BLEND, i);
			} else {
				glDisablei(GL_BLEND, i);
			}
			glBlendFuncSeparatei(i,
				_blend_factor_to_gl_blend_factor(attachment_blend_desc.src_color_blend_factor),
				_blend_factor_to_gl_blend_factor(attachment_blend_desc.dst_color_blend_factor),
				_blend_factor_to_gl_blend_factor(attachment_blend_desc.src_alpha_blend_factor),
				_blend_factor_to_gl_blend_factor(attachment_blend_desc.dst_alpha_blend_factor)
			);
			glBlendEquationSeparatei(i,
				_blend_op_to_gl_blend_op(attachment_blend_desc.color_blend_op),
				_blend_op_to_gl_blend_op(attachment_blend_desc.alpha_blend_op));
		}
	}

	void set_viewports(const Viewport* viewports, unsigned int count) {
		if (count > MAX_VIEWPORTS) {
			count = MAX_VIEWPORTS;
		}
		struct GLViewport {
			GLfloat x;
			GLfloat y;
			GLfloat width;
			GLfloat height;
		};
		struct GLDepthRange {
			GLdouble min_depth;
			GLdouble max_depth;
		};
		GLViewport gl_viewports[MAX_VIEWPORTS];
		GLDepthRange gl_depth_ranges[MAX_VIEWPORTS];
		for (unsigned int i = 0; i < count; ++i) {
			const Viewport& viewport = viewports[i];
			GLViewport& gl_viewport = gl_viewports[i];
			gl_viewport.x = (GLfloat)viewport.x;
			gl_viewport.y = (GLfloat)viewport.y;
			gl_viewport.width = (GLfloat)viewport.width;
			gl_viewport.height = (GLfloat)viewport.height;
			GLDepthRange& gl_depth_range = gl_depth_ranges[i];
			gl_depth_range.min_depth = (GLdouble)viewport.min_depth;
			gl_depth_range.max_depth = (GLdouble)viewport.max_depth;
		}
		glViewportArrayv(0, count, (GLfloat*)gl_viewports);
		glDepthRangeArrayv(0, count, (GLdouble*)gl_depth_ranges);
	}

	void set_scissors(const Rect* scissors, unsigned int count) {
		count = std::min(count, MAX_VIEWPORTS);
		GLint gl_scissors[MAX_VIEWPORTS * 4];
		for (unsigned int i = 0; i < count; ++i) {
			gl_scissors[i * 4 + 0] = scissors[i].x;
			gl_scissors[i * 4 + 1] = scissors[i].y;
			gl_scissors[i * 4 + 2] = scissors[i].width;
			gl_scissors[i * 4 + 3] = scissors[i].height;
		}
		glScissorArrayv(0, count, gl_scissors);
	}

	void set_scissor_test_enabled(bool enable) {
		if (enable) {
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}

	Primitives _primitives = Primitives::TriangleList;

	void set_primitives(Primitives primitives) {
		_primitives = primitives;
	}

	GLenum _primitives_to_gl_primitives(Primitives primitives) {
		switch (primitives) {
		case Primitives::PointList:     return GL_POINTS;
		case Primitives::LineList:      return GL_LINES;
		case Primitives::LineStrip:     return GL_LINE_STRIP;
		case Primitives::TriangleList:  return GL_TRIANGLES;
		case Primitives::TriangleStrip: return GL_TRIANGLE_STRIP;
		default:						return 0; // should never happen
		}
	}

	void draw(unsigned int vertex_count, unsigned int vertex_offset) {
		glDrawArrays(_primitives_to_gl_primitives(_primitives), vertex_offset, vertex_count);
	}

	void draw_indexed(unsigned int index_count, unsigned int base_vertex) {
		glDrawElementsBaseVertex(_primitives_to_gl_primitives(_primitives), index_count, GL_UNSIGNED_INT, nullptr, base_vertex);
	}

} // namespace api
} // namespace graphics

#endif // GRAPHICS_API_OPENGL