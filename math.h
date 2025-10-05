#pragma once
#include "vec2.h"

// CONSTANTS

inline constexpr float M_PI      = 3.141592654f;
inline constexpr float M_2PI     = 6.283185307f;
inline constexpr float M_1DIVPI  = 0.318309886f;
inline constexpr float M_1DIV2PI = 0.159154943f;
inline constexpr float M_PIDIV2  = 1.570796327f;
inline constexpr float M_PIDIV4  = 0.785398163f;

// SCALARS

float smoothstep(float x);
float smootherstep(float x);
float lerp(float a, float b, float t);
float lerp_angle(float a, float b, float t);

// VECTORS

// All functions are safe to call on zero vectors, and all angles are in radians.

float length_squared(const Vec2f& v);
float length(const Vec2f& v);
Vec2f unit(float angle);
Vec2f normalize(const Vec2f& v);
Vec2f abs(const Vec2f& v);
Vec2f perp(const Vec2f& v); // rotates v 90 deg clockwise
float dot(const Vec2f& a, const Vec2f& b); // Dot product
float det(const Vec2f& a, const Vec2f& b); // Determinant, aka 2D cross product
Vec2f complex_conjugate(const Vec2f& v);
Vec2f complex_product(const Vec2f& a, const Vec2f& b);
Vec2f complex_square(const Vec2f& v);
float angle_unsigned(const Vec2f& a, const Vec2f& b);
float angle_signed(const Vec2f& a, const Vec2f& b);
bool clockwise(const Vec2f& a, const Vec2f& b); // True if b is clockwise of a
Vec2f rotate(const Vec2f& v, float angle);
Vec2f min(const Vec2f& a, const Vec2f& b);
Vec2f max(const Vec2f& a, const Vec2f& b);
Vec2f lerp(const Vec2f& a, const Vec2f& b, float t);
Vec2f lerp_polar(const Vec2f& a, const Vec2f& b, float t);
Vec2f damp(const Vec2f& a, const Vec2f& b, float damping, float dt); // time-dependent damping
Vec2f clamp(const Vec2f& v, const Vec2f& min, const Vec2f& max);

// COMPUTATIONAL GEOMETRY

bool convex(std::span<const Vec2f> polygon);
std::vector<Vec2f> triangulate(std::span<const Vec2f>polygon); // Returns a list of triangles.
