#pragma once

enum class Direction {
	N,  // north
	NE, // northeast
	E,  // east
	SE, // southeast
	S,  // south
	SW, // southwest
	W,  // west
	NW, // northwest
};

bool is_cardinal(Direction d); // N, E, S, W
bool is_ordinal(Direction d); // NE, SE, SW, NW
Vec2f to_unit(Direction d);
Direction to_cardinal(const Vec2f& v);