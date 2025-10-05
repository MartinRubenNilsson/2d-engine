#pragma once

namespace delaunay {
	// Time complexity: O(n log n) on average, O(n^2) in the worst case
	void build_bowyer_watson(std::span<const Vec2f> points, std::vector<unsigned int>& indices);
}