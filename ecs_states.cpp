#include "stdafx.h"
#include "ecs_states.h"
#include "text_fonts.h"
#include "text.h"

namespace ecs {
	struct StateMachine {
		std::vector<State> states;
		StateId current_state;
		StateId next_state;
		float next_state_transition_time = 0.f;
	};

	StateId add_state(StateMachine& sm, State&& state) {
		StateId handle{ .index = (unsigned int)sm.states.size() };
		sm.states.emplace_back(std::move(state));
		return handle;
	}

	StateId find_state(const StateMachine& sm, std::string_view name) {
		for (unsigned int index = 0; index < sm.states.size(); index++) {
			if (sm.states[index].name == name) {
				return StateId{ index };
			}
		}
		return StateId();
	}

	State* _get_state(StateMachine& sm, StateId handle) {
		if (handle.index >= sm.states.size())
			return nullptr;
		return &sm.states[handle.index];
	}

	bool transition(StateMachine& sm, StateId state, entt::entity entity) {
		State* next = _get_state(sm, state);
		if (!next) return false;
		// PITFALL: The enter/exit callbacks might want to set a delayed transition,
		// so it's important that we reset these BEFORE calling the callbacks.
		sm.next_state = StateId();
		sm.next_state_transition_time = 0.f;
		// Exit current state if we're in one.
		if (State* curr = _get_state(sm, sm.current_state)) {
			if (curr->stop) {
				curr->stop(entity);
			}
			// PITFALL: Reset the time *after* calling exit since
			// the callback might want to query it.
			curr->time_active = 0.f;
		}
		// Enter next state.
		next->time_active = 0.f; // DEFENSIVE
		if (next->start) {
			next->start(entity);
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

	void transition_later(StateMachine& sm, StateId state, float time) {
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

	std::string_view get_current_state(entt::entity entity) {
		if (!_registry.all_of<StateMachine>(entity))
			return {};
		StateMachine& sm = _registry.get<StateMachine>(entity);
		State* state = _get_state(sm, sm.current_state);
		if (!state)
			return {};
		return state->name;
	}

	bool transition_to_state(entt::entity entity, std::string_view name) {
		if (!_registry.all_of<StateMachine>(entity))
			return false;
		StateMachine& sm = _registry.get<StateMachine>(entity);
		StateId state = find_state(sm, name);
		if (state == StateId())
			return false;
		return transition(sm, state, entity);
	}

	void transition_to_state_later(entt::entity entity, std::string_view name, float time) {
		if (!_registry.all_of<StateMachine>(entity))
			return;
		StateMachine& sm = _registry.get<StateMachine>(entity);
		StateId state = find_state(sm, name);
		if (state == StateId())
			return;
		transition_later(sm, state, time);
	}

	void handle(entt::entity entity, const StateEvent& ev) {
		if (StateMachine* sm = _registry.try_get<StateMachine>(entity)) {
			handle(*sm, entity, ev);
		}
	}

	void update_state_machines(float dt) {
		for (auto [entity, sm] : _registry.view<StateMachine>().each()) {
			update(sm, entity, dt);
		}
	}

	void debug_draw_state_machines() {
		text::Text text{};
		text.font = text::load_font("assets/fonts/Helvetica.ttf");;
		text.letter_height = 6.f;
		text.origin = text::TextOrigin::UpperCenter;

		for (auto [entity, sm, body] : _registry.view<StateMachine, b2BodyId>().each()) {
			std::string string;
			if (State* current_state = _get_state(sm, sm.current_state)) {
				string += current_state->name;
			}
			if (State* next_state = _get_state(sm, sm.next_state)) {
				string += " -> ";
				string += next_state->name;
			}
			if (string.empty()) continue;
			text.string.assign(string.begin(), string.end());
			text.position = b2Body_GetWorldCenterOfMass(body);
			text.position.y += 8.f;
			text::draw_later(text);
		}

		text::draw_all_now("ecs::debug_draw_state_machines()");
	}
}
