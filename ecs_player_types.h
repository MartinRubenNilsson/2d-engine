#pragma once

namespace ecs {
	enum PLAYER_TILE_ID { // player.tsx
		PLAYER_TILE_ID_IDLE_S = 0,
		PLAYER_TILE_ID_IDLE_N = 16,
		PLAYER_TILE_ID_IDLE_E = 32,
		PLAYER_TILE_ID_PUSH_S = 8,
		PLAYER_TILE_ID_PUSH_N = 24,
		PLAYER_TILE_ID_PUSH_E = 40,
		PLAYER_TILE_ID_WALK_S = 48,
		PLAYER_TILE_ID_WALK_N = 52,
		PLAYER_TILE_ID_WALK_E = 64,
		PLAYER_TILE_ID_RUN_S = 51,
		PLAYER_TILE_ID_RUN_N = 55,
		PLAYER_TILE_ID_RUN_E = 70,
		PLAYER_TILE_ID_SLASH_S = 132,
		PLAYER_TILE_ID_SLASH_N = 148,
		PLAYER_TILE_ID_SLASH_E = 164,
		PLAYER_TILE_ID_SLASH_DONE_S = 12,
		PLAYER_TILE_ID_SLASH_DONE_N = 28,
		PLAYER_TILE_ID_SLASH_DONE_E = 44,
		PLAYER_TILE_ID_BOW_DRAW_S = 133,
		PLAYER_TILE_ID_BOW_DRAW_N = 149,
		PLAYER_TILE_ID_BOW_DRAW_E = 165,
		PLAYER_TILE_ID_BOW_RELEASE_S = 135,
		PLAYER_TILE_ID_BOW_RELEASE_N = 151,
		PLAYER_TILE_ID_BOW_RELEASE_E = 167,
		PLAYER_TILE_ID_DYING_SE = 178,
		PLAYER_TILE_ID_DYING_NE = 181,
		PLAYER_TILE_ID_DEAD_SE = 180,
		PLAYER_TILE_ID_DEAD_NE = 183,
		PLAYER_TILE_ID_HURT_S = 244,
	};

	// These all happen in the "normal" state.
	enum class PlayerMotion {
		Motionless,
		Walking,
		Running,
		Sneaking,
		Pushing
	};

	struct Player {
		float input_x = 0.f; // one of -1, 0, 1
		float input_y = 0.f; // one of -1, 0, 1
		Vec2f input_dir = Vec2f::ZERO; // either zero or an unit vector

		PlayerMotion motion = PlayerMotion::Motionless;

		int max_health = 3;
		int health = 3;
		int arrows = 10;
		int bombs = 5;
		int rupees = 10;

		float invincibility_time = 0.f;
	};
}