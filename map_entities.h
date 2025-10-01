#pragma once

namespace tiled {
	struct Map;
}

namespace map {
	void destroy_entities();
	void create_entities(const tiled::Map& map);
}
