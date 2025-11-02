#include "stdafx.h"
#include "window_cursor.h"
#include <GLFW/glfw3.h>

namespace window {
	extern GLFWwindow* _window;

	bool get_cursor_visible() {
		return glfwGetInputMode(_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
	}

	void set_cursor_visible(bool visible) {
		glfwSetInputMode(_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
	}

	Vec2d get_cursor_position() {
		Vec2d position;
		glfwGetCursorPos(_window, &position.x, &position.y);
		return position;
	}

	void set_cursor_position(const Vec2d& position) {
		glfwSetCursorPos(_window, position.x, position.y);
	}

	CursorId arrow_cursor{};
	CursorId ibeam_cursor{};
	CursorId crosshair_cursor{};
	CursorId hand_cursor{};
	CursorId hresize_cursor{};
	CursorId vresize_cursor{};

	std::vector<GLFWcursor*> _cursors;
	CursorId _current_cursor{};

	void _create_standard_cursors() {
		arrow_cursor.id = (uint16_t)_cursors.size();
		_cursors.push_back(glfwCreateStandardCursor(GLFW_ARROW_CURSOR));
		ibeam_cursor.id = (uint16_t)_cursors.size();
		_cursors.push_back(glfwCreateStandardCursor(GLFW_IBEAM_CURSOR));
		crosshair_cursor.id = (uint16_t)_cursors.size();
		_cursors.push_back(glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR));
		hand_cursor.id = (uint16_t)_cursors.size();
		_cursors.push_back(glfwCreateStandardCursor(GLFW_HAND_CURSOR));
		hresize_cursor.id = (uint16_t)_cursors.size();
		_cursors.push_back(glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR));
		vresize_cursor.id = (uint16_t)_cursors.size();
		_cursors.push_back(glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
	}

	void _destroy_cursors() {
		for (GLFWcursor* cursor : _cursors) {
			glfwDestroyCursor(cursor);
		}
		_cursors.clear();
		_current_cursor = {};
	}

	CursorId create_cursor(const Vec2u& size, uint8_t* pixels, const Vec2i& hotspot) {
		GLFWimage image{};
		image.width = (int)size.x;
		image.height = (int)size.y;
		image.pixels = pixels;
		GLFWcursor* cursor = glfwCreateCursor(&image, hotspot.x, hotspot.y);
		if (!cursor) return {};
		const CursorId id{ .id = (uint16_t)_cursors.size() };
		_cursors.push_back(cursor);
		return id;
	}

	CursorId get_current_cursor() {
		return _current_cursor;
	}

	void set_cursor(CursorId cursor) {
		if (cursor.id >= _cursors.size())
			return;
		if (cursor.id == _current_cursor.id)
			return;
		glfwSetCursor(_window, _cursors[cursor.id]);
		_current_cursor = cursor;
	}
}