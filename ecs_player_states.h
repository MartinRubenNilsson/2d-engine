#pragma once

namespace ecs {
	struct StateId;
	struct StateMachine;

	StateId add_player_normal_state(StateMachine& sm, StateId parent);
	StateId add_player_slashing_state(StateMachine& sm, StateId parent);
	StateId add_player_shooting_state(StateMachine& sm, StateId parent);
	StateId add_player_hurt_state(StateMachine& sm, StateId parent);
	StateId add_player_dying_state(StateMachine& sm);
	StateId add_player_dead_state(StateMachine& sm);
}