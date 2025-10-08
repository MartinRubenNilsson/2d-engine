#pragma once

namespace graphics {
	extern Handle<VertexShader> sprite_vert;
	extern Handle<FragmentShader> sprite_frag;
	extern Handle<Texture> error_texture;
}

namespace sprites {
	enum SPRITE_FLAGS : uint8_t {
		SPRITE_VISIBLE           = (1 << 0), // for use in game logic
		SPRITE_FLIP_HORIZONTALLY = (1 << 1),
		SPRITE_FLIP_VERTICALLY   = (1 << 2),
		SPRITE_FLIP_DIAGONALLY   = (1 << 3),
		SPRITE_ROTATE_90         = SPRITE_FLIP_HORIZONTALLY | SPRITE_FLIP_DIAGONALLY,
		SPRITE_ROTATE_180        = SPRITE_FLIP_HORIZONTALLY | SPRITE_FLIP_VERTICALLY,
		SPRITE_ROTATE_270        = SPRITE_FLIP_VERTICALLY | SPRITE_FLIP_DIAGONALLY,
	};

	constexpr size_t L1_CACHE_LINE_SIZE = std::hardware_constructive_interference_size;

	struct /*alignas(L1_CACHE_LINE_SIZE)*/ Sprite {
		Handle<graphics::VertexShader> vertex_shader = graphics::sprite_vert;
		Handle<graphics::FragmentShader> fragment_shader = graphics::sprite_frag;
		Handle<graphics::Texture> texture = graphics::error_texture;
		Handle<graphics::Buffer> uniform_buffer;
		uint16_t uniform_buffer_size = 0; // size in bytes of the uniform buffer
		uint16_t uniform_buffer_offset = 0; // offset in bytes into the uniform buffer; must be a multiple of 256
		Vec2f position; // top-left corner position in world space
		Vec2f size; // width and height in world space
		Vec2f tex_position = { 0.f, 0.f }; // top-left corner in normalized texture coordinates
		Vec2f tex_size = { 1.f, 1.f }; // width and height in normalized texture coordinates
		Vec2f sorting_point; // relative to position
		Color color = colors::WHITE;
		uint8_t sorting_layer = 0;
		uint8_t flags = SPRITE_VISIBLE;
		uint8_t _unused = 0;
	};

	//static_assert(sizeof(Sprite) == L1_CACHE_LINE_SIZE, "Sprite doesn't fit exactly in a cache line!");

	// Orders sprites by draw order. Members are compared in a cascading fashion:
	// 
	// 1. sorting_layer
	// 2. position.y + sorting_point.y
	// 3. position.x + sorting_point.x
	// 4. vertex shader
	// 5. fragment shader
	// 5. texture
	// 6. uniform buffer
	// 7. uniform buffer size
	// 8. uniform buffer offset
	// 
	// In all cases, a less-than comparison is used.
	// If one case yields a tie, then the next case is considered, and so on.
	//
	bool operator<(const Sprite& left, const Sprite& right);

	// Adds a sprite to be sorted and drawn later.
	void draw_later(const Sprite& sprite);
	// Sorts all added sprites by draw order.
	void sort_all();
	// Draws all added sprites and removes them.
	void draw_all_now();

	void clear_statistics();
	unsigned int get_num_sprites_drawn();
	unsigned int get_num_batches_drawn();
	unsigned int get_num_sprites_in_largest_batch();
	unsigned int get_num_vertices_in_largest_batch();
}