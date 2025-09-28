#pragma once

namespace ecs {
	// Stores shader uniform data for custom sprite shaders.
	struct UniformBlock {
		// The size of this data block (256 bytes = 64 floats = 4 mat4s)
		// has been chosen to match the minimum alignment requirement when
		// binding uniform buffers using glBindBufferRange(). You don't have
		// to fill all bytes, just memcpy however much data the shader needs.
		unsigned char data[256] = {};
	};

	UniformBlock& emplace_uniform_block(entt::entity entity);
	UniformBlock& emplace_uniform_block(entt::entity entity, const void* data, size_t size);
}