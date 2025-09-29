#include "stdafx.h"
#include "ecs_states.h"
#include "fonts.h"
#include "text.h"

namespace ecs {
	struct StateMachine {
		std::vector<State> states;
		StateHandle current_state;
		StateHandle next_state;
		float next_state_transition_time = 0.f;
	};

	StateHandle add_state(StateMachine& sm, State&& state) {
		StateHandle handle{ .index = (unsigned int)sm.states.size() };
		sm.states.emplace_back(std::move(state));
		return handle;
	}

	StateHandle find_state(const StateMachine& sm, std::string_view id) {
		for (unsigned int index = 0; index < sm.states.size(); index++) {
			if (sm.states[index].id == id) {
				return StateHandle{ index };
			}
		}
		return StateHandle();
	}

	State* _get_state(StateMachine& sm, StateHandle handle) {
		if (handle.index >= sm.states.size())
			return nullptr;
		return &sm.states[handle.index];
	}

	bool transition(StateMachine& sm, StateHandle state, entt::entity entity) {
		State* next = _get_state(sm, state);
		if (!next) return false;
		// PITFALL: The enter/exit callbacks might want to set a delayed transition,
		// so it's important that we reset these BEFORE calling the callbacks.
		sm.next_state = StateHandle();
		sm.next_state_transition_time = 0.f;
		// Exit current state if we're in one.
		if (State* curr = _get_state(sm, sm.current_state)) {
			if (curr->exit) {
				curr->exit(entity);
			}
			// PITFALL: Reset the time *after* calling exit since
			// the callback might want to query it.
			curr->time_active = 0.f;
		}
		// Enter next state.
		next->time_active = 0.f; // DEFENSIVE
		if (next->enter) {
			next->enter(entity);
		}
		sm.current_state = state;
		return true;
	}

	void update(StateMachine& sm, entt::entity entity, float dt) {
		if (sm.next_state_transition_time > 0.f) {
			sm.next_state_transition_time -= dt;
			if (sm.next_state_transition_time <= 0.f) {
				transition(sm, sm.next_state, entity);
			}
		}
		State* s = _get_state(sm, sm.current_state);
		if (!s) return;
		s->time_active += dt;
		if (!s->update) return;
		s->update(entity, dt);
	}

	void handle(StateMachine& sm, entt::entity entity, const StateEvent& event) {
		State* s = _get_state(sm, sm.current_state);
		if (!s) return;
		if (!s->handle) return;
		s->handle(entity, event);
	}

	void transition_later(StateMachine& sm, StateHandle state, float time) {
		if (state.index >= sm.states.size())
			return; // invalid state handle
		if (time <= 0.f)
			return; // invalid time
		if (0.f < sm.next_state_transition_time && sm.next_state_transition_time < time)
			return; // another transition is scheduled to happen earlier
		sm.next_state = state;
		sm.next_state_transition_time = time;
	}

	extern entt::registry _registry;

	StateMachine& emplace_state_machine(entt::entity entity) {
		return _registry.emplace_or_replace<StateMachine>(entity);
	}

	bool transition_to_state(entt::entity entity, std::string_view state_id) {
		if (!_registry.all_of<StateMachine>(entity))
			return false;
		StateMachine& sm = _registry.get<StateMachine>(entity);
		StateHandle state = find_state(sm, state_id);
		if (state == StateHandle())
			return false;
		return transition(sm, state, entity);
	}

	void transition_to_state_later(entt::entity entity, std::string_view state_id, float time) {
		if (!_registry.all_of<StateMachine>(entity))
			return;
		StateMachine& sm = _registry.get<StateMachine>(entity);
		StateHandle state = find_state(sm, state_id);
		if (state == StateHandle())
			return;
		transition_later(sm, state, time);
	}

	void update_state_machines(float dt) {
		for (auto [entity, sm] : _registry.view<StateMachine>().each()) {
			update(sm, entity, dt);
		}
	}

	void debug_draw_state_machines() {
		text::Text text{};
		text.font = fonts::load_font("assets/fonts/Helvetica.ttf");;
		text.pixel_height = 48.f;
		text.scale = { 0.1f, 0.1f };
		for (auto [entity, sm, body] : _registry.view<StateMachine, b2BodyId>().each()) {
			std::string string;
			if (State* current_state = _get_state(sm, sm.current_state)) {
				string += current_state->id;
			}
			if (State* next_state = _get_state(sm, sm.next_state)) {
				string += " -> ";
				string += next_state->id;
			}
			if (string.empty()) continue;
			text.string = text::to_u32(string);
			text.position = b2Body_GetPosition(body) + Vector2f(-8.f, -10.f);
			text::render(text);
		}
	}
}
