#pragma once
#include "vec2.h"

template <typename T>
struct Rect2 {
	Vec2<T> min{};
	Vec2<T> max{};
};

template <typename T>
bool empty(const Rect2<T>& r) {
	return r.min.x > r.max.x || r.min.y > r.max.y;
}

// TODO
//template <typename T>
//Rect2<T> intersection(const Rect2<T>& r1, const Rect2<T>& r2) {
//	return { }
//}

using Rect2i = Rect2<int>;
using Rect2u = Rect2<unsigned int>;
using Rect2f = Rect2<float>;
using Rect2d = Rect2<double>;