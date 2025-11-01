#pragma once

template <typename T>
struct Handle {
	uint16_t index = 0;
	uint16_t version = 0; // Valid versions start at 1.

	auto operator<=>(const Handle&) const = default;
};