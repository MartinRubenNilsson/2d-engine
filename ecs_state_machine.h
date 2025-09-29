#pragma once

namespace ecs {
	struct StateHandle {
		unsigned int index = UINT_MAX;
		auto operator<=>(const StateHandle&) const = default;
	};

	struct StateEvent {
		unsigned int type = UINT_MAX;
		const void* data = nullptr;
	};

	struct State {
		std::string_view id;
		void (*enter)(entt::entity entity) = nullptr;
		void (*exit)(entt::entity entity) = nullptr;
		void (*update)(entt::entity entity, float dt) = nullptr;
		void (*handle)(entt::entity entity, const StateEvent& event) = nullptr;
		float time_active = 0.f; // since last enter
	};

	struct StateMachine; // implementation is private

	StateHandle add_state(StateMachine& sm, State&& state);
	StateHandle find_state(const StateMachine& sm, std::string_view id);
	bool transition(StateMachine& sm, StateHandle state, entt::entity entity); // immediately
	void transition_later(StateMachine& sm, StateHandle state, float time);
	void update(StateMachine& sm, entt::entity entity, float dt);
	void handle(StateMachine& sm, entt::entity entity, const StateEvent& event);

	StateMachine& emplace_state_machine(entt::entity entity);
	bool transition_to_state(entt::entity entity, std::string_view state_id); // immediately
	void transition_to_state_later(entt::entity entity, std::string_view state_id, float time);

	void update_state_machines(float dt);
	void debug_draw_state_machines();
}