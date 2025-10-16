#pragma once

namespace text {
	struct FontId {
		uint16_t id = UINT16_MAX;

		auto operator<=>(const FontId&) const = default;
		operator bool() const; // checks if the ID is valid
	};
}