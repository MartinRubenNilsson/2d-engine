#include "stdafx.h"
#include "ui_textboxes.h"
#include "input.h"
#include "audio.h"
#include <deque>

namespace ui {
namespace textboxes {

	std::vector<Textbox> _textboxes; // Sorted by Textbox::path.

	void add_textbox(Textbox&& textbox) {
		_textboxes.emplace_back(std::move(textbox));
	}

	std::span<const Textbox> get_textboxes() {
		return _textboxes;
	}

	std::span<const Textbox> get_textboxes_starting_with(std::string_view path) {
		const auto [first, last] = std::equal_range(_textboxes.begin(), _textboxes.end(), Textbox{ path },
			[](const Textbox& a, const Textbox& b) {
				const size_t size = std::min(a.path.size(), b.path.size());
				return strncmp(a.path.data(), b.path.data(), size) < 0;
			});
		return { first, last };
	}

	// Returns true if the character at pos is plain text, defined as a character that is not inside a <...> tag.
	bool _is_plain(std::string_view string, size_t pos) {
		for (size_t i = pos; i < string.size(); ++i) {
			if (string[i] == '<') return i != pos;
			if (string[i] == '>') return false;
		}
		return true;
	}

	// Counts the number of plain text characters in the string.
	size_t _get_plain_count(std::string_view string) {
		size_t count = 0;
		for (size_t i = 0; i < string.size(); ++i) {
			if (_is_plain(string, i)) {
				++count;
			}
		}
		return count;
	}

	// Returns the nth plain text character if n < _get_plain_length(string), or '\0' otherwise.
	char _get_nth_plain(std::string_view string, size_t n) {
		size_t count = 0;
		for (size_t i = 0; i < string.size(); ++i) {
			if (_is_plain(string, i)) {
				if (count == n) return string[i];
				++count;
			}
		}
		return '\0';
	}

	// Replaces graphical plain text with non-breaking spaces, starting at offset.
	// This is used to prevent the text from jumping around when being typed out.
	std::string _replace_graphical_plain_with_nbsp(std::string_view string, size_t offset) {
		static constexpr char NBSP_utf8[] = { (char)0xC2, (char)0xA0, '\0' }; // non-breaking space
		static constexpr std::string_view NBSP = NBSP_utf8;
		std::string ret;
		size_t count = 0;
		for (size_t i = 0; i < string.size(); ++i) {
			bool replace = false;
			if (_is_plain(string, i)) {
				if (count >= offset && isgraph(string[i])) {
					replace = true;
				}
				++count;
			}
			ret += replace ? NBSP : string.substr(i, 1);
		}
		return ret;
	}

	enum class State {
		Closed,
		Opening,
		Open,
	};

	State _state = State::Closed;
	Textbox _current_textbox{}; // The current textbox (may be empty).
	std::deque<Textbox> _next_textboxes; // Queue of next textboxes to open.
	std::string _typed_text; // The text revealed so far in the open textbox.
	float _typed_time = 0.f; // Time since last character in _typed_text was typed
	size_t _typed_count = 0; // number of characters typed

	bool closed() {
		return _state == State::Closed;
	}

	const Textbox& get_current_textbox() {
		return _current_textbox;
	}

	std::string_view get_typed_text() {
		return _typed_text;
	}

	bool is_typing_text() {
		return _typed_count < _get_plain_count(_current_textbox.text);
	}

	void skip_typing_text() {
		_typed_count = _get_plain_count(_current_textbox.text);
	}

	size_t _current_option = 0;
	bool _current_option_selected = false;

	void set_current_option(size_t option) {
		if (option >= _current_textbox.options.size())
			return;
		if (option != _current_option) {
			audio::create_event("event:/ui/snd_button_hover");
		}
		_current_option = option;
	}

	size_t get_current_option() {
		return _current_option;
	}

	void select_current_option() {
		if (_current_option >= _current_textbox.options.size())
			return;
		_current_option_selected = true;
	}

	void close() {
		_state = State::Closed;
		_current_textbox = {};
		_typed_text.clear();
		_typed_time = 0.f;
		_typed_count = 0;
		_current_option = 0;
		_current_option_selected = false;
	}

	void close_all() {
		close();
		_next_textboxes.clear();
	}

	void open(const Textbox& textbox) {
		close();
		_state = State::Opening;
		_current_textbox = textbox;
		if (!_current_textbox.opening_sound.empty()) {
			audio::create_event(_current_textbox.opening_sound);
		}
	}

	void open_next(const Textbox& textbox) {
		if (closed()) {
			open(textbox);
		} else {
			_next_textboxes.push_back(textbox);
		}
	}

	void open_next(std::string_view path) {
		for (const Textbox& textbox : get_textboxes_starting_with(path)) {
			open_next(textbox);
		}
	}

	void proceed() {
		if (_next_textboxes.empty()) {
			close();
			return;
		}
		open(_next_textboxes.front());
		_next_textboxes.pop_front();
	}

	void _add_textboxes(); // ui_textbox_creation.cpp

	void startup() {
		_textboxes.clear();
		_add_textboxes();
		std::sort(_textboxes.begin(), _textboxes.end(), [](const Textbox& a, const Textbox& b) {
			return a.path < b.path; }); // Sort all presets by path.
	}

	void _on_pressed_confirm() {
		if (is_typing_text()) {
			skip_typing_text();
		} else if (!_current_textbox.options.empty()) {
			select_current_option();
		} else {
			proceed();
		}
	}

	void _on_pressed_up() {
		if (_current_option > 0) {
			set_current_option(_current_option - 1);
		}
	}

	void _on_pressed_down() {
		if (_current_option + 1 < _current_textbox.options.size()) {
			set_current_option(_current_option + 1);
		}
	}

	void _handle_input() {
		if (input::pressed(window::Key::C)) {
			_on_pressed_confirm();
		}
		if (input::pressed(window::Key::Up)) {
			_on_pressed_up();
		}
		if (input::pressed(window::Key::Down)) {
			_on_pressed_down();
		}
	}

	void _update_typing(float dt) {
		const size_t plain_count = _get_plain_count(_current_textbox.text);
		if (_typed_count < plain_count && _current_textbox.typing_speed > 0.f) {
			float seconds_per_char = 1.f / _current_textbox.typing_speed;
			_typed_time += dt;
			if (_typed_time >= seconds_per_char) {
				_typed_time -= seconds_per_char;
				if (isgraph(_get_nth_plain(_current_textbox.text, _typed_count))) {
					audio::create_event(_current_textbox.typing_sound);
				}
				++_typed_count;
			}
		} else {
			_typed_count = plain_count;
		}
		_typed_text = _replace_graphical_plain_with_nbsp(_current_textbox.text, _typed_count);
	}

	void _update_options() {
		if (_current_option >= _current_textbox.options.size())
			return;
		if (!_current_option_selected)
			return;
		_current_option_selected = false;
		audio::create_event("event:/ui/snd_button_click");
		if (_current_textbox.options[_current_option].on_selected) {
			_current_textbox.options[_current_option].on_selected();
		}
		proceed();
	}

	void update(float dt) {
		switch (_state) {
			case State::Closed: {
				proceed();
			} break;
			case State::Opening: {
				_state = State::Open;
			} break;
			case State::Open: {
				_handle_input();
				_update_options();
				_update_typing(dt);
			} break;
		}
	}
}
}