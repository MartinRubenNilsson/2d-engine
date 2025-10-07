#pragma once

namespace ecs {
	// Should cast the component to the appropriate type, (deep) copy it and emplace it on the clone.
	using CloningHandler = void(*)(entt::entity clone, const void* component);

	void set_cloning_handler(entt::type_info type, CloningHandler handler);

	entt::entity clone(entt::entity entity);
}