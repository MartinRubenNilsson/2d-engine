#pragma once
#include <string_view>
#include <initializer_list>
#include <span>

namespace graphics {

	struct ShaderDesc {
		std::string_view debug_name = "shader";
		// The shader code can be:
		// - A string containing the source code (e.g., GLSL, HLSL)
		// - Bytecode for a compiled shader (e.g., SPIR-V, DXBC, DXIL)
		std::span<const unsigned char> code;
		bool binary = false; // True if bytecode, false if source code.
	};

	enum class Format {
		UNKNOWN,
		R8_UNORM,
		RG8_UNORM,
		RGB8_UNORM,
		RGBA8_UNORM,
		R32_FLOAT,
		RG32_FLOAT,
		RGB32_FLOAT,
		RGBA32_FLOAT,
	};

	struct VertexInputAttribDesc {
		//unsigned int location = 0;
		unsigned int binding = 0;
		Format format = Format::UNKNOWN;
		unsigned int offset = 0;
		bool normalized = false; //TODO: remove, not supported in d3d11
	};

	struct VertexInputDesc {
		std::string_view debug_name = "vertex input";
		std::initializer_list<VertexInputAttribDesc> attributes;
		std::span<const unsigned char> bytecode; // Only needed for D3D11
	};

	enum class BufferType {
		VertexBuffer,
		IndexBuffer,
		UniformBuffer,
	};

	struct BufferDesc {
		std::string_view debug_name = "buffer";
		unsigned int size = 0; // in bytes
		BufferType type = BufferType::VertexBuffer;
		bool dynamic = false; // If true, the buffer can be updated with update_buffer().
		const void* initial_data = nullptr;
	};

	struct TextureDesc {
		std::string_view debug_name = "texture";
		unsigned int width = 0;
		unsigned int height = 0;
		Format format = Format::UNKNOWN;
		const void* initial_data = nullptr;
		bool framebuffer_color = false; // If true, the texture can be used as a framebuffer color attachment.
	};

	enum class Filter {
		Nearest, // Sample nearest texel
		Linear, // Linear interpolation between texels
	};

	enum class Wrap {
		Repeat, // Repeat the texture
		MirroredRepeat, // Repeat with mirroring
		ClampToEdge, // Clamp to the edge
		ClampToBorder, // Clamp to a border color
		MirrorClampToEdge, // Mirror the texture once and then clamp to the edge
	};

	struct SamplerDesc {
		std::string_view debug_name = "sampler";
		Filter filter = Filter::Nearest;
		Wrap wrap = Wrap::Repeat;
		float border_color[4] = {};
	};

	struct FramebufferDesc {
		std::string_view debug_name = "framebuffer";
	};

	enum class PolygonMode {
		Fill, // Draw the polygon with a solid color
		Line, // Draw the polygon as a wireframe
	};

	enum class CullMode {
		None, // No triangles are discarded
		Front, // Front-facing triangles are discarded
		Back, // Back-facing triangles are discarded
	};

	struct RasterizerDesc {
		std::string_view debug_name = "rasterizer state";
		PolygonMode polygon_mode = PolygonMode::Fill;
		CullMode cull_mode = CullMode::Back;
		bool front_face_ccw = false; // If true, the front face is counter-clockwise
	};

	enum class BlendFactor {
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		DstColor,
		OneMinusDstColor,
		SrcAlpha,
		OneMinusSrcAlpha,
		DstAlpha,
		OneMinusDstAlpha,
		// NOTE: there are more options availible, but I haven't added them yet
	};

	enum class BlendOp {
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max,
	};

	struct AttachmentBlendDesc {
		bool blend_enable = false;
		BlendFactor src_color_blend_factor = BlendFactor::One;
		BlendFactor dst_color_blend_factor = BlendFactor::Zero;
		BlendOp color_blend_op = BlendOp::Add;
		BlendFactor src_alpha_blend_factor = BlendFactor::One;
		BlendFactor dst_alpha_blend_factor = BlendFactor::Zero;
		BlendOp alpha_blend_op = BlendOp::Add;
	};

	struct BlendDesc {
		std::string_view debug_name = "blend state";
		std::initializer_list<AttachmentBlendDesc> attachments;
	};

	enum class Primitives {
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
	};

	struct Viewport {
		float x = 0.f;
		float y = 0.f;
		float width = 0.f;
		float height = 0.f;
		float min_depth = 0.f;
		float max_depth = 1.f;
	};

	struct Rect {
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};
}