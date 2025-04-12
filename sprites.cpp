#include "stdafx.h"
#include "sprites.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"

namespace sprites {
	bool operator<(const Sprite& left, const Sprite& right) {
		if (left.sorting_layer != right.sorting_layer) {
			return left.sorting_layer < right.sorting_layer;
		}
		if (left.position.y + left.sorting_point.y != right.position.y + right.sorting_point.y) {
			return left.position.y + left.sorting_point.y < right.position.y + right.sorting_point.y;
		}
		if (left.position.x + left.sorting_point.x != right.position.x + right.sorting_point.x) {
			return left.position.x + left.sorting_point.x < right.position.x + right.sorting_point.x;
		}
		if (left.shader != right.shader) {
			return left.shader < right.shader;
		}
		if (left.texture != right.texture) {
			return left.texture < right.texture;
		}
		if (left.uniform_buffer != right.uniform_buffer) {
			return left.uniform_buffer < right.uniform_buffer;
		}
		if (left.uniform_buffer_size != right.uniform_buffer_size) {
			return left.uniform_buffer_size < right.uniform_buffer_size;
		}
		if (left.uniform_buffer_offset != right.uniform_buffer_offset) {
			return left.uniform_buffer_offset < right.uniform_buffer_offset;
		}
		return false;
	}

	struct Batch {
		Handle<graphics::Shader> shader;
		Handle<graphics::Texture> texture;
		Handle<graphics::Buffer> uniform_buffer;
		unsigned int uniform_buffer_size = 0;
		unsigned int uniform_buffer_offset = 0;
		unsigned int sprite_count = 0; // not used for drawing, only for debugging
		unsigned int vertex_count = 0;
		unsigned int vertex_offset = 0; // offset into the vertex buffer
	};

	eastl::vector<Sprite> _sprites;
	eastl::vector<Batch> _batches;

	unsigned int _sprites_drawn = 0;
	unsigned int _batches_drawn = 0;
	unsigned int _largest_batch_sprite_count = 0;
	unsigned int _largest_batch_vertex_count = 0;

	void add(const Sprite& sprite) {
		_sprites.push_back(sprite);
	}

	void sort() {
		if (_sprites.empty()) return;

		// Sort sprites by draw order.
		// 
		// When investigating, I found that the added sprites usually already containes sorted runs,
		// so I tried using eastl::tim_sort_buffer to take advantage of that. It gave me a quite
		// significant performance boost.
		// 
		// Timsort requires a scratch buffer of size N/2, so we reserve this amount
		// at the end of _sprites. This way, _sprites.end() will be both the end of
		// the array of sprites and also the beginning of the scratch buffer.

		_sprites.reserve(_sprites.size() + _sprites.size() / 2);
		eastl::tim_sort_buffer(_sprites.begin(), _sprites.end(), _sprites.end());
	}

	void draw() {
		if (_sprites.empty()) return;

		// Sprites sharing the same state (shader, texture, etc.) are batched together to reduce draw calls.
		// This is done by creating a triangle strip for each batch and drawing it only when the state changes.
		// Each sprite is represented by 4 vertices in the triangle strip, but we also need to add duplicate
		// vertices to create degenerate triangles that separate the sprites in the strip: If ABCD and EFGH
		// are the triangle strips for two sprites, then the batched triangle strip will be ABCDDEEFGH.

		graphics::temp_vertices.clear();

		for (const Sprite& sprite : _sprites) {

			Vector2f tl = sprite.position; // top-left
			Vector2f br = sprite.position + sprite.size; // bottom-right
			Vector2f tr = { br.x, tl.y }; // top-right
			Vector2f bl = { tl.x, br.y }; // bottom-left

			const Vector2f tex_tl = sprite.tex_position;
			const Vector2f tex_br = sprite.tex_position + sprite.tex_size;
			const Vector2f tex_tr = { tex_br.x, tex_tl.y };
			const Vector2f tex_bl = { tex_tl.x, tex_br.y };

			if (sprite.flags & SPRITE_FLIP_HORIZONTALLY) {
				std::swap(tl, tr);
				std::swap(bl, br);
			}
			if (sprite.flags & SPRITE_FLIP_VERTICALLY) {
				std::swap(tl, bl);
				std::swap(tr, br);
			}
			if (sprite.flags & SPRITE_FLIP_DIAGONALLY) {
				std::swap(bl, tr);
			}

			if (_batches.empty()) {
				Batch& first_batch = _batches.emplace_back();
				first_batch.shader = sprite.shader;
				first_batch.texture = sprite.texture;
				first_batch.uniform_buffer = sprite.uniform_buffer;
				first_batch.uniform_buffer_size = sprite.uniform_buffer_size;
				first_batch.uniform_buffer_offset = sprite.uniform_buffer_offset;
			} else {
				Batch& current_batch = _batches.back();
				if (sprite.shader == current_batch.shader &&
					sprite.texture == current_batch.texture &&
					sprite.uniform_buffer == current_batch.uniform_buffer &&
					sprite.uniform_buffer_size == current_batch.uniform_buffer_size &&
					sprite.uniform_buffer_offset == current_batch.uniform_buffer_offset) {
					// Add degenerate triangles to separate the sprites
					graphics::temp_vertices.emplace_back(graphics::temp_vertices.back()); // D
					graphics::temp_vertices.emplace_back(tl, sprite.color, tex_tl); // E
					current_batch.vertex_count += 2;
				} else {
					Batch& new_batch = _batches.emplace_back();
					new_batch.shader = sprite.shader;
					new_batch.texture = sprite.texture;
					new_batch.uniform_buffer = sprite.uniform_buffer;
					new_batch.uniform_buffer_size = sprite.uniform_buffer_size;
					new_batch.uniform_buffer_offset = sprite.uniform_buffer_offset;
					new_batch.vertex_offset = (unsigned int)graphics::temp_vertices.size();
				}
			}

			// Add the vertices of the new sprite to the batch
			graphics::temp_vertices.emplace_back(tl, sprite.color, tex_tl);
			graphics::temp_vertices.emplace_back(bl, sprite.color, tex_bl);
			graphics::temp_vertices.emplace_back(tr, sprite.color, tex_tr);
			graphics::temp_vertices.emplace_back(br, sprite.color, tex_br);

			// Update statistics
			_batches.back().sprite_count += 1;
			_batches.back().vertex_count += 4;
		}

		//TODO: hide this logic in wrapper functions
		const unsigned int vertices_byte_size = (unsigned int)graphics::temp_vertices.size() * sizeof(graphics::Vertex);
		if (vertices_byte_size <= graphics::get_buffer_size(graphics::dynamic_vertex_buffer)) {
			graphics::update_buffer(graphics::dynamic_vertex_buffer, graphics::temp_vertices.data(), vertices_byte_size);
		} else {
			graphics::recreate_buffer(graphics::dynamic_vertex_buffer, vertices_byte_size, graphics::temp_vertices.data());
		}

		Handle<graphics::Shader> last_bound_shader;
		Handle<graphics::Texture> last_bound_texture;
		Handle<graphics::Buffer> last_bound_uniform_buffer;
		unsigned int last_bound_uniform_buffer_size = 0;
		unsigned int last_bound_uniform_buffer_offset = 0;

		graphics::set_primitives(graphics::Primitives::TriangleStrip);

		for (const Batch& batch : _batches) {
			if (batch.shader != last_bound_shader) {
				graphics::bind_shader(batch.shader);
				last_bound_shader = batch.shader;
			}
			if (batch.texture != last_bound_texture) {
				graphics::bind_texture(0, batch.texture);
				last_bound_texture = batch.texture;
			}
			if (batch.uniform_buffer != last_bound_uniform_buffer ||
				batch.uniform_buffer_size != last_bound_uniform_buffer_size ||
				batch.uniform_buffer_offset != last_bound_uniform_buffer_offset) {
				graphics::bind_uniform_buffer_range(1, batch.uniform_buffer,
					batch.uniform_buffer_size, batch.uniform_buffer_offset);
				last_bound_uniform_buffer = batch.uniform_buffer;
			}
			
			graphics::draw(batch.vertex_count, batch.vertex_offset);
			_largest_batch_sprite_count = std::max(_largest_batch_sprite_count, batch.sprite_count);
			_largest_batch_vertex_count = std::max(_largest_batch_vertex_count, batch.vertex_count);
		}

		_sprites_drawn += (unsigned int)_sprites.size();
		_batches_drawn += (unsigned int)_batches.size();

		_sprites.clear();
		_batches.clear();
		graphics::temp_vertices.clear();
	}

	void clear_drawing_statistics() {
		_sprites_drawn = 0;
		_batches_drawn = 0;
		_largest_batch_sprite_count = 0;
		_largest_batch_vertex_count = 0;
	}

	unsigned int get_sprites_drawn() {
		return _sprites_drawn;
	}

	unsigned int get_batches_drawn() {
		return _batches_drawn;
	}

	unsigned int get_largest_batch_sprite_count() {
		return _largest_batch_sprite_count;
	}

	unsigned int get_largest_batch_vertex_count() {
		return _largest_batch_vertex_count;
	}
}
