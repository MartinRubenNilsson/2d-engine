#pragma once

namespace ecs {
	struct MapId {
		uint16_t id = UINT16_MAX;

		operator bool() const; // checks if the ID is valid
		auto operator<=>(const MapId&) const = default;
	};

	struct TilesetId {
		uint16_t id = UINT16_MAX;

		operator bool() const; // checks if the ID is valid
		auto operator<=>(const TilesetId&) const = default;
	};

	struct TileId { // must match tiled::TileId exactly!
		union {
			uint32_t value = 0x0FFFFFFF; // set flip flags to 0 and remaining bits to 1
			struct {
				uint32_t id : 16;
				uint32_t tileset_id : 12;
				uint32_t rotated_hexagonal_120 : 1; // only for hexagonal maps
				uint32_t flipped_diagonally : 1;
				uint32_t flipped_vertically : 1;
				uint32_t flipped_horizontally : 1;
			};
		};

		explicit operator bool() const; // checks if both IDs are valid
		auto operator<=>(const TileId& other) const {
			return value <=> other.value;
		}
		bool operator==(const TileId& other) const {
			return value == other.value;
		}
	};

	struct ObjectId {
		uint32_t id = UINT32_MAX;

		operator bool() const; // checks if the ID is valid
		auto operator<=>(const ObjectId&) const = default;
	};
}