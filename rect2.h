#pragma once
#include "vec2.h"

template <typename T>
struct Rect2 {
	Vec2<T> min{};
	Vec2<T> max{};
};

template <typename T>
bool empty(const Vec2<T>& min, const Vec2<T>& max) {
	return min.x > max.x || min.y > max.y;
}

template <typename T>
bool empty(const Rect2<T>& r) {
	return empty(r.min, r.max);
}

template <typename T>
bool contains(const Rect2<T>& r, const Vec2<T>& p) {
	if (empty(r.min, p))
		return false;
	if (empty(p, r.max))
		return false;
	return true;
}

template <typename T>
bool contains(const Rect2<T>& r1, const Rect2<T>& r2) {
	if (empty(r2))
		return true; // every set contains the empty set
	if (empty(r1.min, r2.min))
		return false;
	if (empty(r2.max, r1.max))
		return false;
	return true;
}

template <typename T>
bool intersects(const Rect2<T>& r1, const Rect2<T>& r2) {
	return false; //TODO
}

template <typename T>
Rect2<T> intersection(const Rect2<T>& r1, const Rect2<T>& r2) {
	return { }; // TODO
}

using Rect2i = Rect2<int>;
using Rect2u = Rect2<unsigned int>;
using Rect2f = Rect2<float>;
using Rect2d = Rect2<double>;