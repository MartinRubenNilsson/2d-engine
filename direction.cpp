#include "stdafx.h"
#include "direction.h"

bool is_cardinal(Direction d) {
	switch (d) {
		case Direction::N: [[fallthrough]];
		case Direction::E: [[fallthrough]];
		case Direction::S: [[fallthrough]];
		case Direction::W: return true;
		default: return false;
	}
}

bool is_ordinal(Direction d) {
	switch (d) {
		case Direction::NE: [[fallthrough]];
		case Direction::SE: [[fallthrough]];
		case Direction::SW: [[fallthrough]];
		case Direction::NW: return true;
		default: return false;
	}
}

Vec2f to_unit(Direction d) {
	constexpr float leg = 0.7071067811865475; // = 1/sqrt(2)
	switch (d) {
		case Direction::N:  return { 0.f, -1.f };
		case Direction::NE: return { leg, -leg };
		case Direction::E:  return { 1.f, 0.f };
		case Direction::SE: return { leg, leg };
		case Direction::S:  return { 0.f, 1.f };
		case Direction::SW: return { -leg, leg };
		case Direction::W:  return { -1.f, 0.f };
		case Direction::NW: return { -leg, -leg };
		default: return { 0.f, -1.f }; // This should never happen.
	}
}

Direction to_cardinal(Vec2f v) {
	if (v.x >= +abs(v.y)) return Direction::E;
	if (v.x <= -abs(v.y)) return Direction::W;
	if (v.y >= +abs(v.x)) return Direction::S;
	if (v.y <= -abs(v.x)) return Direction::N;
	return Direction::N; // This should never happen.
}
