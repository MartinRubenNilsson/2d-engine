#pragma once

namespace ecs
{
	enum class VfxType
	{
		Explosion,
	};

	entt::entity create_vfx(VfxType type, const Vec2f& position);
}
