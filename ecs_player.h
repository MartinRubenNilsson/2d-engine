#pragma once
#include "timer.h"

namespace ecs {
	struct PhysicsEvent;

	enum INPUT_FLAGS : unsigned int {
		// Continuous actions

		INPUT_LEFT = (1 << 0),
		INPUT_RIGHT = (1 << 1),
		INPUT_UP = (1 << 2),
		INPUT_DOWN = (1 << 3),
		INPUT_RUN = (1 << 4),
		INPUT_STEALTH = (1 << 5),

		// One-shot actions

		INPUT_INTERACT = (1 << 6),
		INPUT_SWING_SWORD = (1 << 7),
		INPUT_SHOOT_BOW = (1 << 8),
		INPUT_DROP_BOMB = (1 << 9),
	};

	enum class PlayerState {
		Normal,
		SwingingSword,
		ShootingBow,
		Dying,
		Dead
	};

	struct Player {
		unsigned int input_flags = 0;
		PlayerState state = PlayerState::Normal;
		Vector2f look_dir = { 0.f, 1.f };
		Timer hurt_timer = { 1.f };
		int max_health = 3;
		int health = 3;
		int arrows = 10;
		int bombs = 5;
		int rupees = 10;
		entt::entity held_item = entt::null;
		bool touching_pushable_block = false;

		// It really is the pushable block that's making the sound,
		// but I can't be bothered to make a separate component for it rn.
		Handle<audio::Event> stone_sliding_sound;
	};

	void handle_window_event_for_players(const window::Event& ev);
	void update_players(float dt);
	void show_player_debug_window();

	// Returns the first entity with a Player component, or entt::null if none exists.
	// Try to avoid using this function, as it can lead to spaghetti code.
	entt::entity find_player_entity();

	Player& emplace_player(entt::entity entity, const Player& player = {});
	Player* get_player(entt::entity entity);
	bool remove_player(entt::entity entity);
	bool has_player(entt::entity entity);

	bool kill_player(entt::entity entity);

	void on_player_physics_event(const PhysicsEvent& ev);

	struct Damage;

	bool apply_damage_to_player(entt::entity entity, const Damage& damage);
}
