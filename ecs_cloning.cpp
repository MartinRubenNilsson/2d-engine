#include "stdafx.h"
#include "ecs_cloning.h"

namespace std {
	template <>
	struct hash<entt::type_info> {
		size_t operator()(const entt::type_info& type) const {
			return type.hash();
		}
	};
}

namespace ecs {

	std::unordered_map<entt::type_info, CloningHandler> _cloning_handler;

	void set_cloning_handler(entt::type_info type, CloningHandler handler) {
		_cloning_handler.emplace(type, handler);
	}

	extern entt::registry _registry;

	entt::entity clone(entt::entity entity) {
		if (!_registry.valid(entity))
			return entt::null;
		const entt::entity clone = _registry.create();
		for (auto [name, storage] : _registry.storage()) {
			if (!storage.contains(entity))
				continue;
			auto handler = _cloning_handler.find(storage.type());
			if (handler != _cloning_handler.end()) {
				handler->second(clone, storage.value(entity));
			} else {
				storage.push(clone, storage.value(entity));
			}
		}
		return clone;
	}
}