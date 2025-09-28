#include "stdafx.h"
#include "ecs_uniform_block.h"

namespace ecs {
	extern entt::registry _registry;

    UniformBlock& emplace_uniform_block(entt::entity entity) {
		return _registry.emplace_or_replace<UniformBlock>(entity);
    }

    UniformBlock& emplace_uniform_block(entt::entity entity, const void* data, size_t size) {
		assert(size <= sizeof(UniformBlock));
		UniformBlock& block = emplace_uniform_block(entity);
		memcpy(block.data, data, size);
		return block;
    }
}