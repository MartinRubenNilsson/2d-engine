#pragma once

namespace tiled {
	struct Object;
}

namespace ecs {

	// Wrapper class for safe and convenient access to a Tiled object.
	class TiledObject {
		const tiled::Object* _obj;

	public:
		TiledObject(const tiled::Object& obj);

		// Returns the position of the top-left corner, unless the object is a tile object,
		// in which case it returns the position of the bottom-left corner.
		Vector2f get_position() const;
		Vector2f get_top_left() const;

		std::string_view get_string(std::string_view name) const;
		int get_int(std::string_view name) const;
		float get_float(std::string_view name) const;
		bool get_bool(std::string_view name) const;
		Color get_color(std::string_view name) const;
		std::string_view get_file(std::string_view name) const;
		entt::entity get_object(std::string_view name) const;
	};

	void emplace_tiled_object(entt::entity entity, const tiled::Object& obj);
	void clear_all_tiled_objects();
}