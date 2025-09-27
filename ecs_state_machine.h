#pragma once

namespace ecs {
	struct StateHandle {
		unsigned int index = UINT_MAX;
		auto operator<=>(const StateHandle&) const = default;
	};

	using StateEnterCallback = void (*)(entt::entity entity);
	using StateExitCallback = void (*)(entt::entity entity);
	using StateUpdateCallback = void (*)(entt::entity entity, float dt);
	using StateProcessCallback = void (*)(entt::entity entity, unsigned int event_type, const void* event_data);

	struct State {
		std::string_view id;
		StateEnterCallback enter = nullptr;
		StateExitCallback exit = nullptr;
		StateUpdateCallback update = nullptr;
		StateProcessCallback process = nullptr;
		float time_active = 0.f; // since last enter
	};

	struct StateMachine; // implementation is private

	StateHandle add_state(StateMachine& sm, State&& state);
	bool transition(StateMachine& sm, StateHandle state, entt::entity entity); // transitions immediately
	bool transition(StateMachine& sm, std::string_view state_id, entt::entity entity); // transitions immediately
	void update(StateMachine& sm, entt::entity entity, float dt);
	void process(StateMachine& sm, entt::entity entity, unsigned int event_type, const void* event_data);

	StateMachine& emplace_state_machine(entt::entity entity);
	bool transition_state_machine(entt::entity entity, std::string_view state_id); // transitions immediately
	void transition_state_machine_later(entt::entity entity, std::string_view state_id, float time);
	void update_state_machines(float dt);
	void debug_draw_state_machines();
}