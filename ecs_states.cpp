#include "stdafx.h"
#include "ecs_states.h"
#include "text.h"
#include "text_fonts.h"
#include "graphics_debugging.h"

namespace ecs {
	struct StateMachine {
		std::vector<State> states;
		StateId current_state;
		StateId next_state;
		float next_state_transition_time = 0.f;
	};

	StateId add_state(StateMachine& sm, State&& state) {
		StateId id{ .index = (unsigned int)sm.states.size() };
		sm.states.emplace_back(std::move(state));
		return id;
	}

	StateId find_state(const StateMachine& sm, std::string_view name) {
		for (unsigned int index = 0; index < sm.states.size(); index++) {
			if (sm.states[index].name == name) {
				return StateId{ index };
			}
		}
		return StateId();
	}

	// Returns nullptr for invalid states.
	State* _get_state(StateMachine& sm, StateId state) {
		if (state.index >= sm.states.size())
			return nullptr;
		return &sm.states[state.index];
	}

	StateId _get_parent(const StateMachine& sm, StateId state) {
		if (state.index >= sm.states.size())
			return {};
		return sm.states[state.index].parent;
	}

	// If either of a and b are null or if they do not have a common ancestor, returns the null ID.
	StateId _lowest_common_ancestor(const StateMachine& sm, StateId a, StateId b) {
		if (a == StateId() || b == StateId()) {
			return StateId();
		} else if (a == b) {
			return a;
		} else if (a < b) {
			return _lowest_common_ancestor(sm, a, _get_parent(sm, b));
		} else {
			return _lowest_common_ancestor(sm, _get_parent(sm, a), b);
		}
	}

	// Stops begin, then stops its parent, then its parent's parent, and so on until end is reached.
	// End itself is not stopped. If begin is the null state then nothing is done. If end is the null
	// state or if begin is not a descendant of end, then begin and all its ascendants will be stopped.
	void _stop_up_to(StateMachine& sm, StateId begin, StateId end, entt::entity entity) {
		if (begin == StateId()) return; // Begin is the null state -> nothing to stop.
		if (begin == end) return; // Exclude end.
		State* s = _get_state(sm, begin);
		if (s->stop) {
			s->stop(entity); // First stop the state itself...
		}
		_stop_up_to(sm, s->parent, end, entity); // ...then stop its parent, and so on.
	}

	// Stops a child of begin, then stops its child's child, and so on until end is reached. Begin itself
	// is not stopped, but end is. If end is the null state then nothing is done. If begin is the null
	// state or if begin is not an ascendant of end, then end and all its ascendants will be stopped.
	void _start_down_to(StateMachine& sm, StateId begin, StateId end, entt::entity entity) {
		if (end == StateId()) return; // End is the null state -> nothing to start.
		if (end == begin) return; // Exclude begin.
		State* s = _get_state(sm, end);
		_stop_up_to(sm, begin, s->parent, entity); // First start the parent state, and so on...
		if (s->start) {
			s->start(entity); // ...then start the state itself.
		}
	}

	// Stops states bottom-to-top from (and including) a up to and excluding the LCA of a and b,
	// then starts states top-to-bottom from (and excluding) the LCA down to (and including) b.
	// Safe to call even if a and/or b are null or if a and b do not have a common ancestor.
	void _transition_recursively(StateMachine& sm, StateId a, StateId b, entt::entity entity) {
		const StateId lca = _lowest_common_ancestor(sm, a, b); // May be the null ID!
		// Stop all states from (and including) a up to (but not including) lca.
		_stop_up_to(sm, a, lca, entity); 
		// Starts all states from (but not including) lca down to (and including) b.
		_start_down_to(sm, lca, b, entity);
	}

	bool transition(StateMachine& sm, StateId state, entt::entity entity) {
		State* next = _get_state(sm, state);
		if (!next) return false;
		// PITFALL: The start/stop callbacks might want to set a delayed transition,
		// so it's important that we reset these BEFORE calling _transition_recursively().
		sm.next_state = StateId();
		sm.next_state_transition_time = 0.f;
		_transition_recursively(sm, sm.current_state, state, entity);
		sm.current_state = state;
		return true;
	}

	// Recursively updates all the state's parents before updating the state itself,
	// so that the first state updated is the top state, then one of its children, and
	// so on until we reach and update the state with which the function was called.
	void _update_top_to_bottom(StateMachine& sm, StateId state, entt::entity entity, float dt) {
		State* s = _get_state(sm, state);
		if (!s) return;
		_update_top_to_bottom(sm, s->parent, entity, dt); // Update all parents first...
		if (!s->update) return;
		s->update(entity, dt); // ...then finally the child state.
	}

	void update(StateMachine& sm, entt::entity entity, float dt) {
		if (sm.next_state_transition_time > 0.f) {
			sm.next_state_transition_time -= dt;
			if (sm.next_state_transition_time <= 0.f) {
				transition(sm, sm.next_state, entity);
			}
		}
		_update_top_to_bottom(sm, sm.current_state, entity, dt);
	}

	// Recursively lets the the state's parents handle the event before the state,
	// so that the first handler is the top state, then one of its children, and
	// so on until the state with which the function was called handles the event.
	void _handle_top_to_bottom(StateMachine& sm, StateId state, entt::entity entity, const StateEvent& event) {
		State* s = _get_state(sm, state);
		if (!s) return;
		_handle_top_to_bottom(sm, s->parent, entity, event); // Let all parents handle the event first...
		if (!s->handle) return;
		s->handle(entity, event); // ...then finally let the child state handle the event.
	}

	void handle(StateMachine& sm, entt::entity entity, const StateEvent& event) {
		_handle_top_to_bottom(sm, sm.current_state, entity, event);
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
		GRAPHICS_DEBUG_GROUP;

		text::Text text{};
		text.font = text::load_font("assets/fonts/Helvetica.ttf");;
		text.font_size = 6.f;
		text.anchor = text::TextAnchor::UpperCenter;

		for (auto [entity, sm, body] : _registry.view<StateMachine, b2BodyId>().each()) {
			std::string string;
			if (State* current_state = _get_state(sm, sm.current_state)) {
				string += current_state->name;
			}
			if (string.empty()) continue;
			text.string.assign(string.begin(), string.end());
			text.position = b2Body_GetWorldCenterOfMass(body);
			text.position.y += 8.f;
			text::draw_later(text);
		}

		text::draw_all_now();
	}
}
