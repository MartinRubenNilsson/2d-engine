#pragma once

namespace ecs {

	unsigned int get_tileset_id(std::string_view name);
	Handle<graphics::Texture> get_tileset_texture(unsigned int tileset_id);

	struct TileAnimation {
		unsigned int tileset_id = UINT_MAX; // index into tiled::Context::tilesets[]
		unsigned int tile_id = UINT_MAX; // index into tiled::Tileset::tiles[]
		unsigned int _previous_tile_id = UINT_MAX;
		unsigned int _animated_tile_id = UINT_MAX;
		unsigned int _frame_id = 0; // index into tiled::Tile::animation[]
		float progress = 0.f; // aka normalized time, in the range [0, 1]
		float speed = 1.f;
		bool loop = true;
		bool _dirty = false;
		bool _looped = false;
	};

	struct FlipbookAnimation {
		unsigned int rows = 0;
		unsigned int columns = 0;
		float fps = 0.f;
		float time = 0.f;
	};

	void setup_tile_animations();
	void update_tile_animations(float dt);
	void update_flipbook_animations(float dt);
	void update_animated_sprites(float dt);

	TileAnimation& emplace_tile_animation(entt::entity entity);
	TileAnimation* get_tile_animation(entt::entity entity);

	FlipbookAnimation& emplace_flipbook_animation(entt::entity entity);
}
