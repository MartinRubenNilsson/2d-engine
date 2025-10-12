#pragma once

namespace ecs {
	struct StateId {
		unsigned int index = UINT_MAX;
		auto operator<=>(const StateId&) const = default;
	};

	struct StateEvent {
		// "type" is by design not a shared enum - so you can define your own.
		unsigned int type = UINT_MAX;
		const void* data = nullptr;
	};

	struct State {
		std::string_view name;
		StateId parent{}; // For hierarchical states.
		void (*start)(entt::entity entity) = nullptr;
		void (*stop)(entt::entity entity) = nullptr;
		void (*update)(entt::entity entity, float dt) = nullptr;
		void (*handle)(entt::entity entity, const StateEvent& ev) = nullptr;
	};

	struct StateMachine; // implementation is private

	StateId add_state(StateMachine& sm, State&& state);
	StateId find_state(const StateMachine& sm, std::string_view name);
	bool transition(StateMachine& sm, StateId state, entt::entity entity); // immediately
	void transition_later(StateMachine& sm, StateId state, float time);
	void update(StateMachine& sm, entt::entity entity, float dt);
	void handle(StateMachine& sm, entt::entity entity, const StateEvent& ev);

	StateMachine& emplace_state_machine(entt::entity entity);
	std::string_view get_current_state(entt::entity entity);
	bool transition_to_state(entt::entity entity, std::string_view name); // immediately
	void transition_to_state_later(entt::entity entity, std::string_view name, float time);
	void handle(entt::entity entity, const StateEvent& ev);

	void update_state_machines(float dt);
	void debug_draw_state_machines();
}