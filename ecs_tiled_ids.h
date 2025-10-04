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

	struct TileId {
		uint16_t id = UINT16_MAX;
		uint16_t tileset_id = UINT16_MAX;

		operator bool() const; // checks if both IDs are valid
		auto operator<=>(const TileId&) const = default;
	};

	struct ObjectId {
		uint32_t id = UINT32_MAX;

		operator bool() const; // checks if the ID is valid
		auto operator<=>(const ObjectId&) const = default;
	};
}