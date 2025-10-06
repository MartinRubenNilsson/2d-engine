#pragma once

namespace ecs {
	// The collision category bits. Normally you would just set one bit.
	enum CollisionCategory : uint32_t {
		CC_None = 0,
		CC_Default = 1 << 0,
		CC_Player = 1 << 1,
		CC_Enemy = 1 << 2,
		CC_PlayerAttack = 1 << 3,
	};

	// The collision mask bits. This states the categories that the shape would accept for collision.
	enum CollisionMask : uint32_t {
		CM_Default = ~CC_None, // everything
		CM_Player = ~CC_PlayerAttack, // everything except player attack
		CM_Enemy = ~CC_Enemy, // everything except enemy
	};

	// Collision groups allow a certain group of objects to never collide (negative)
	// or always collide (positive). A group index of zero has no effect.
	// Non-zero group filtering always wins against the mask bits.
	enum CollisionGroup : int32_t {
		CG_Default = 0,
	};

	b2Filter get_physics_filter(enum class Tag tag);
}
