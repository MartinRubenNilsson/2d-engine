#include "stdafx.h"
#include "window_events.h"

namespace window {
	std::vector<Event> _events;

	std::span<const Event> get_events() {
		return _events;
	}

	void clear_events() {
		_events.clear();
	}

	void add_event(const Event& ev) {
		_events.push_back(ev);
	}
}
