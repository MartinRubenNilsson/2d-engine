#pragma once

namespace ecs {
	enum class PickupType {
		None,
		Arrow,
		Rupee,
		Bomb,
		Heart,
		Count,
	};

	entt::entity create_pickup(PickupType type, const Vec2f& position);
	PickupType get_pickup_type(entt::entity entity);

	void update_pickups(float dt);
}
