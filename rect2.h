#pragma once
#include "vec2.h"

template <typename T>
struct Rect2 {
	Vec2<T> min{};
	Vec2<T> max{};

	template <typename U>
	constexpr operator Rect2<U>() const {
		return { static_cast<Vec2<U>>(min), static_cast<Vec2<U>>(max) };
	}

	constexpr auto operator<=>(const Rect2&) const = default;

	static const Rect2 ZERO;
	static const Rect2 EMPTY;
};

template <typename T> const Rect2<T> Rect2<T>::ZERO{ Vec2<T>::ZERO, Vec2<T>::ZERO };
template <typename T> const Rect2<T> Rect2<T>::EMPTY{ Vec2<T>::MAX, Vec2<T>::MIN };

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
	if (r1.max.x < r2.min.x)
		return false;
	if (r1.max.y < r2.min.y)
		return false;
	if (r2.max.x < r1.min.x)
		return false;
	if (r2.max.y < r1.min.y)
		return false;
	return true;
}

template <typename T>
Rect2<T> intersection(const Rect2<T>& r1, const Rect2<T>& r2) {
	return { { std::max(r1.min.x, r2.min.x), std::max(r1.min.y, r2.min.y) },
			 { std::min(r1.max.x, r2.max.x), std::min(r1.max.y, r2.max.y) } };
}

// Returns the smallest rect that contains both r1 and r2. This is usually called the "union"
// of the two rects, but that keyword is reserved by C++.
template <typename T>
Rect2<T> join(const Rect2<T>& r1, const Rect2<T>& r2) {
	return { { std::min(r1.min.x, r2.min.x), std::min(r1.min.y, r2.min.y) },
			 { std::max(r1.max.x, r2.max.x), std::max(r1.max.y, r2.max.y) } };
}

template <typename T>
Rect2<T> translate(const Rect2<T>& r, const Vec2<T>& translation) {
	return { r.min + translation, r.max + translation };
}

// Returns the smallest rect that contains all the intermediate rects as r is translated.
template <typename T>
Rect2<T> sweep(const Rect2<T>& r, const Vec2<T>& translation) {
	// TODO: This could probably be optimized quite a bit.
	return join(r, translate(r, translation));
}

using Rect2i = Rect2<int>;
using Rect2u = Rect2<unsigned int>;
using Rect2f = Rect2<float>;
using Rect2d = Rect2<double>;