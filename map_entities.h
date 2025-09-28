#pragma once

namespace tiled {
	struct Map;
}

namespace map {
	struct MapPatch {
		std::vector<entt::entity> destroyed_entities;
		std::vector<entt::entity> opened_chests;
	};

	void destroy_entities();
	void create_entities(const tiled::Map& map);
	void patch_entities(const MapPatch& patch);
}
