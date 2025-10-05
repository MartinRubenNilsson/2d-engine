#pragma once
#include "vec2.h"
#include <span>

namespace kdtree {
	// Builds a k-d tree from a set of points. Complexity: O(n log n)
	void build(std::span<Vec2f> kdtree);

	// Queries the nearest point to a given point in the k-d tree. Sets 'nearest_index' to the index of the nearest point
	// in the k-d tree, and 'nearest_distance_sq' to the squared distance to that point. You can set 'nearest_distance_sq'
	// < FLT_MAX to restrict the search to points within a certain distance. Sets 'nearest_index' to SIZE_MAX if no point
	// is found within the distance, or if the tree is empty. Complexity: O(log n)
	void query_nearest(std::span<const Vec2f> kdtree, const Vec2f& point, size_t& nearest_index, float& nearest_distance_sq);
}