#pragma once

namespace window {
	bool get_cursor_visible();
	void set_cursor_visible(bool visible);
	Vec2d get_cursor_position();
	void set_cursor_position(const Vec2d& position);

	struct CursorId {
		uint16_t id = UINT16_MAX;
	};

	extern CursorId arrow_cursor;
	extern CursorId ibeam_cursor;
	extern CursorId crosshair_cursor;
	extern CursorId hand_cursor;
	extern CursorId hresize_cursor;
	extern CursorId vresize_cursor;

	CursorId create_cursor(const Vec2u& size, uint8_t* pixels, const Vec2i& hotspot = Vec2i::ZERO);
	CursorId get_current_cursor();
	void set_cursor(CursorId cursor);
}