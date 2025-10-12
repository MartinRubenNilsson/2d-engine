#pragma once

struct b2Vec2;

template <typename T>
struct Vec2 {
	T x{};
	T y{};

	constexpr Vec2() = default;
	constexpr Vec2(T x, T y) : x(x), y(y) {}
	constexpr Vec2(const b2Vec2& v) : x(v.x), y(v.y) {}

	template <typename U>
	constexpr operator Vec2<U>() const {
		return { static_cast<U>(x), static_cast<U>(y) };
	}

	constexpr operator b2Vec2() const {
		return { static_cast<float>(x), static_cast<float>(y) };
	}

	constexpr auto operator<=>(const Vec2<T>&) const = default;

	static const Vec2 ZERO;
	static const Vec2 UNIT_X;
	static const Vec2 UNIT_Y;
};

template <typename T> const Vec2<T> Vec2<T>::ZERO{ 0, 0 };
template <typename T> const Vec2<T> Vec2<T>::UNIT_X{ 1, 0 };
template <typename T> const Vec2<T> Vec2<T>::UNIT_Y{ 0, 1 };

template <typename T>
constexpr Vec2<T> operator-(const Vec2<T>& right) {
	return { -right.x, -right.y };
}

template <typename T>
constexpr Vec2<T>& operator+=(Vec2<T>& left, const Vec2<T>& right) {
	left.x += right.x;
	left.y += right.y;
	return left;
}

template <typename T>
constexpr Vec2<T>& operator-=(Vec2<T>& left, const Vec2<T>& right) {
	left.x -= right.x;
	left.y -= right.y;
	return left;
}

template <typename T>
constexpr Vec2<T>& operator*=(Vec2<T>& left, const Vec2<T>& right) {
	left.x *= right.x;
	left.y *= right.y;
	return left;
}

template <typename T>
constexpr Vec2<T>& operator*=(Vec2<T>& left, const T& right) {
	left.x *= right;
	left.y *= right;
	return left;
}

template <typename T>
constexpr Vec2<T>& operator/=(Vec2<T>& left, const Vec2<T>& right) {
	left.x /= right.x;
	left.y /= right.y;
	return left;
}

template <typename T>
constexpr Vec2<T>& operator/=(Vec2<T>& left, const T& right) {
	left.x /= right;
	left.y /= right;
	return left;
}

template <typename T>
constexpr Vec2<T> operator+(const Vec2<T>& left, const Vec2<T>& right) {
	return { left.x + right.x, left.y + right.y };
}

template <typename T>
constexpr Vec2<T> operator-(const Vec2<T>& left, const Vec2<T>& right) {
	return { left.x - right.x, left.y - right.y };
}

template <typename T>
constexpr Vec2<T> operator*(const Vec2<T>& left, const Vec2<T>& right) {
	return { left.x * right.x, left.y * right.y };
}

template <typename T>
constexpr Vec2<T> operator*(const Vec2<T>& left, const T& right) {
	return { left.x * right, left.y * right };
}

template <typename T>
constexpr Vec2<T> operator*(const T& left, const Vec2<T>& right) {
	return { left * right.x, left * right.y };
}

template <typename T>
constexpr Vec2<T> operator/(const Vec2<T>& left, const Vec2<T>& right) {
	return { left.x / right.x, left.y / right.y };
}

template <typename T>
constexpr Vec2<T> operator/(const Vec2<T>& left, const T& right) {
	return { left.x / right, left.y / right };
}

using Vec2i = Vec2<int>;
using Vec2u = Vec2<unsigned int>;
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;