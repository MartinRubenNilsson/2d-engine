#pragma once

namespace ecs {
	struct StateHandle {
		unsigned int index = UINT_MAX;
		auto operator<=>(const StateHandle&) const = default;
	};

	struct State {
		std::string_view id;
		void (*enter)(entt::entity entity) = nullptr;
		void (*exit)(entt::entity entity) = nullptr;
		void (*update)(entt::entity entity, float dt) = nullptr;
		void (*handle)(entt::entity entity, unsigned int event_type, const void* event_data) = nullptr;
		float time_active = 0.f; // since last enter
	};

	struct StateMachine; // implementation is private

	StateHandle add_state(StateMachine& sm, State&& state);
	bool transition(StateMachine& sm, StateHandle state, entt::entity entity); // immediately
	bool transition(StateMachine& sm, std::string_view state_id, entt::entity entity); // immediately
	void update(StateMachine& sm, entt::entity entity, float dt);
	void handle(StateMachine& sm, entt::entity entity, unsigned int event_type, const void* event_data);

	StateMachine& emplace_state_machine(entt::entity entity);
	bool transition_state_machine(entt::entity entity, std::string_view state_id); // immediately
	void transition_state_machine_later(entt::entity entity, std::string_view state_id, float time);
	void update_state_machines(float dt);
	void debug_draw_state_machines();
}