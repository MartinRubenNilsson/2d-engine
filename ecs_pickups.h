#pragma once
#include "timer.h"

namespace ecs
{
	enum class PickupType
	{
		Arrow,
		Rupee,
		Bomb,
		Heart,
		Count,
	};

	struct Pickup
	{
		PickupType type = PickupType::Arrow;
		Timer timer = { 3.f };

	};

	void update_pickups(float dt);

	entt::entity create_pickup(PickupType type, const Vec2f& position);

	Pickup& emplace_pickup(entt::entity entity);
	Pickup* get_pickup(entt::entity entity);
}
