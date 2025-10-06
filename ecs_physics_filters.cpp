#include "stdafx.h"
#include "ecs_physics_filters.h"
#include "ecs_tags.h"

namespace ecs {
	b2Filter get_physics_filter(Tag tag) {
		b2Filter filter = b2DefaultFilter();
		switch (tag) {
			case Tag::Player:
				filter.categoryBits = CC_Player;
				filter.maskBits = CM_Player;
				break;
			case Tag::Slime:
				filter.categoryBits = CC_Enemy;
				break;
			case Tag::Arrow:
				filter.categoryBits = CC_PlayerAttack;
				break;
		}
		return filter;
	}
}
