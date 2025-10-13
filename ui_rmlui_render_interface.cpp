#include "stdafx.h"
#include "ui_rmlui_render_interface.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "graphics_vertices.h"

namespace ui {
	static_assert(std::is_same_v<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>);

	int _viewport_width = 0;
	int _viewport_height = 0;
	graphics::Viewport _previous_viewport{};
	bool _previous_scissor_test_enabled = false;
	graphics::Rect _previous_scissor{};
	Rml::Matrix4f _transform = Rml::Matrix4f::Identity();
	Rml::Matrix4f _view_proj_matrix = Rml::Matrix4f::Identity();

	Rml::TextureHandle _texture_handle_to_rml(Handle<graphics::Texture> handle) {
		// PITFALL: 0 represents an invalid Rml::TextureHandle.
		return *(Rml::TextureHandle*)&handle;
	}

	Handle<graphics::Texture> _texture_handle_from_rml(Rml::TextureHandle handle) {
		// PITFALL: 0 represents an invalid Rml::TextureHandle.
		return *(Handle<graphics::Texture>*) & handle;
	}

	void set_viewport(int viewport_width, int viewport_height) {
		_viewport_width = viewport_width;
		_viewport_height = viewport_height;
		_view_proj_matrix = Rml::Matrix4f::ProjectOrtho(
			0.0f, (float)viewport_width, 0.f, (float)viewport_height, -1.f, 1.f);
	}

	void prepare_render_state() {
		graphics::push_debug_group("UI");
		graphics::get_viewport(_previous_viewport);
		_previous_scissor_test_enabled = graphics::get_scissor_test_enabled();
		graphics::get_scissor(_previous_scissor);
		graphics::set_viewport({ .width = (float)_viewport_width, .height = (float)_viewport_height });
		graphics::bind_vertex_shader(graphics::ui_vert);
		graphics::bind_fragment_shader(graphics::ui_frag);
		graphics::bind_uniform_buffer(1, graphics::ui_uniform_buffer);
		graphics::set_primitives(graphics::Primitives::TriangleList);
	}

	void restore_render_state() {
		graphics::set_viewport(_previous_viewport);
		graphics::set_scissor_test_enabled(_previous_scissor_test_enabled);
		graphics::set_scissor(_previous_scissor);
		graphics::bind_vertex_buffer(0, graphics::dynamic_vertex_buffer, sizeof(graphics::Vertex));
		graphics::bind_index_buffer(graphics::dynamic_index_buffer);
		graphics::pop_debug_group();
	}

	struct CompiledGeometry {
		Handle<graphics::Buffer> vertex_buffer;
		Handle<graphics::Buffer> index_buffer;
		unsigned int index_count = 0;
	};

	Rml::CompiledGeometryHandle RmlUiRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
		CompiledGeometry* compiled_geometry = new CompiledGeometry();
		compiled_geometry->vertex_buffer = graphics::create_buffer({
			.debug_name = "rmlui vertex buffer",
			.size = (unsigned int)(sizeof(graphics::Vertex) * vertices.size()),
			.type = graphics::BufferType::VertexBuffer,
			.initial_data = vertices.data()
		});
		compiled_geometry->index_buffer = graphics::create_buffer({
			.debug_name = "rmlui index buffer",
			.size = (unsigned int)(sizeof(unsigned int) * indices.size()),
			.type = graphics::BufferType::IndexBuffer,
			.initial_data = indices.data()
		});
		compiled_geometry->index_count = (unsigned int)indices.size();
		return (Rml::CompiledGeometryHandle)compiled_geometry;
	}

	void RmlUiRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) {
		CompiledGeometry* compiled_geometry = (CompiledGeometry *)geometry;
		graphics::bind_vertex_buffer(0, compiled_geometry->vertex_buffer, sizeof(graphics::Vertex));
		graphics::bind_index_buffer(compiled_geometry->index_buffer);
		const Rml::Matrix4f transform = _view_proj_matrix * _transform * Rml::Matrix4f::Translate(translation.x, translation.y, 0.0f);
		graphics::update_buffer(graphics::ui_uniform_buffer, transform.data(), sizeof(Rml::Matrix4f));
		if (texture) {
			graphics::bind_texture(0, _texture_handle_from_rml(texture));
		} else {
			graphics::bind_texture(0, graphics::white_texture);
		}
		graphics::draw_indexed(compiled_geometry->index_count);
	}

	void RmlUiRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
		CompiledGeometry* compiled_geometry = (CompiledGeometry*)geometry;
		graphics::destroy_buffer(compiled_geometry->vertex_buffer);
		graphics::destroy_buffer(compiled_geometry->index_buffer);
		delete compiled_geometry;
	}

	Rml::TextureHandle RmlUiRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
		const Handle<graphics::Texture> texture = graphics::load_texture(source);
		if (texture == Handle<graphics::Texture>()) return Rml::TextureHandle();
		const Vec2u size = graphics::get_texture_size(texture);
		texture_dimensions.x = size.x;
		texture_dimensions.y = size.y;
		return _texture_handle_to_rml(texture);
	}

	Rml::TextureHandle RmlUiRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
		const Handle<graphics::Texture> texture = graphics::create_texture({
			.debug_name = "rmlui texture",
			.width = (unsigned int)source_dimensions.x,
			.height = (unsigned int)source_dimensions.y,
			.format = graphics::Format::RGBA8_UNORM,
			.initial_data = source.data()
		});
		if (texture == Handle<graphics::Texture>()) return Rml::TextureHandle();
		return _texture_handle_to_rml(texture);
	}

	void RmlUiRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle) {
		graphics::destroy_texture(_texture_handle_from_rml(texture_handle));
	}

	void RmlUiRenderInterface::EnableScissorRegion(bool enable) {
		graphics::set_scissor_test_enabled(enable);
	}

	void RmlUiRenderInterface::SetScissorRegion(Rml::Rectanglei region) {
		graphics::set_scissor({
			.x = region.Left(),
			.y = region.Top(),
			.width = region.Width(),
			.height = region.Height()
		});
	}

	void RmlUiRenderInterface::SetTransform(const Rml::Matrix4f* transform) {
		_transform = transform ? *transform : Rml::Matrix4f::Identity();
	}
}
