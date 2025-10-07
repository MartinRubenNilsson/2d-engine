#include "stdafx.h"
#include "window_events.h"

namespace window {
	std::queue<Event> _event_queue;

	void push_event(const Event& ev) {
		_event_queue.push(ev);
	}

	bool pop_event(Event& ev) {
		if (_event_queue.empty())
			return false;
		ev = _event_queue.front();
		_event_queue.pop();
		return true;
	}
}
