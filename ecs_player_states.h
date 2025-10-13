#pragma once

namespace ecs {
	struct StateId;
	struct StateMachine;

	// This state is run as long as the player is alive (not dying or dead).
	StateId add_player_alive_state(StateMachine& sm); 

	// The player is not doing a special action such as attacking, being hurt, etc.
	StateId add_player_normal_state(StateMachine& sm, StateId parent); // parent = alive

	// The player is attacking with the sword.
	StateId add_player_slashing_state(StateMachine& sm, StateId parent); // parent = alive

	// The player is attacking with the bow.
	StateId add_player_shooting_state(StateMachine& sm, StateId parent); // parent = alive

	// The player just got hurt and is reacting to it.
	StateId add_player_hurt_state(StateMachine& sm, StateId parent); // parent = alive

	// The player just lost all health and is about to die.
	StateId add_player_dying_state(StateMachine& sm);

	// The player is lying dead on the ground.
	StateId add_player_dead_state(StateMachine& sm);
}