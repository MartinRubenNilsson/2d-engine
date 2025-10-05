#include "stdafx.h"
#include "kdtree.h"

namespace kdtree {
	bool _compare_x(const Vec2f& a, const Vec2f& b) {
		return a.x < b.x;
	}

	bool _compare_y(const Vec2f& a, const Vec2f& b) {
		return a.y < b.y;
	}

	void _build_recursively(std::span<Vec2f> kdtree, bool compare_x) {
		const size_t size = kdtree.size();
		if (size <= 1) return;
		const size_t left_size = size / 2; // Also the index of the median point
		const size_t right_size = size - left_size - 1;
		std::nth_element(kdtree.begin(), kdtree.begin() + left_size, kdtree.end(), compare_x ? _compare_x : _compare_y);
		_build_recursively(kdtree.first(left_size), !compare_x);
		_build_recursively(kdtree.last(right_size), !compare_x);
	}

	void build(std::span<Vec2f> kdtree) {
		_build_recursively(kdtree, true);
	}

	void _query_nearest_recursive(std::span<const Vec2f> kdtree, const Vec2f& point, const Vec2f** nearest_point, float& nearest_distance_sq, bool compare_x) {
		const size_t size = kdtree.size();
		// 1. If the tree is empty, return immediately.
		if (size == 0) return;
		// 2. If the tree has only one point, check if it is closer than the current nearest point, then return.
		if (size == 1) {
			const float distance_sq = length_squared(kdtree[0] - point);
			if (distance_sq < nearest_distance_sq) {
				*nearest_point = &kdtree[0];
				nearest_distance_sq = distance_sq;
			}
			return;
		}
		size_t mid_point_index = size / 2; // Also the size of the left subtree
		const Vec2f& mid_point = kdtree[mid_point_index];
		const float signed_distance_to_split_plane = compare_x ? mid_point.x - point.x : mid_point.y - point.y;
		std::span<const Vec2f> nearest_subtree = kdtree.first(mid_point_index); // The left subtree
		std::span<const Vec2f> other_subtree = kdtree.last(size - mid_point_index - 1); // The right subtree
		if (signed_distance_to_split_plane < 0.f) {
			// The point is on the right side of the split plane, so swap the subtrees.
			std::swap(nearest_subtree, other_subtree);
		}
		// 3. Query the nearest point in the subtree whose region contains the point.
		_query_nearest_recursive(nearest_subtree, point, nearest_point, nearest_distance_sq, !compare_x);
		// 4. Check if the point on the split plane is closer than the current nearest point.
		const float distance_sq_to_mid_point = length_squared(mid_point - point);
		if (distance_sq_to_mid_point < nearest_distance_sq) {
			*nearest_point = &mid_point;
			nearest_distance_sq = distance_sq_to_mid_point;
		}
		// 5. If the split plane is closer than the current nearest point, there might be points in the other subtree
		// that are closer, so query the other subtree.
		const float distance_sq_to_split_plane = signed_distance_to_split_plane * signed_distance_to_split_plane;
		if (distance_sq_to_split_plane < nearest_distance_sq) {
			_query_nearest_recursive(other_subtree, point, nearest_point, nearest_distance_sq, !compare_x);
		}
	}

	void query_nearest(std::span<const Vec2f> kdtree, const Vec2f& point, size_t& nearest_index, float& nearest_distance_sq) {
		nearest_index = SIZE_MAX;
		const Vec2f* nearest_point = nullptr;
		_query_nearest_recursive(kdtree, point, &nearest_point, nearest_distance_sq, true);
		if (nearest_point) {
			nearest_index = nearest_point - kdtree.data();
		}
	}
}