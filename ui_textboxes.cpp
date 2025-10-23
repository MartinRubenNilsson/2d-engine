#include "stdafx.h"
#include "ui_textboxes.h"
#include "input.h"
#include "audio.h"
#include <deque>

namespace ui {
namespace textboxes {

	std::vector<Textbox> _textboxes; // Sorted by Textbox::path.

	void create_textbox(Textbox&& textbox) {
		_textboxes.emplace_back(std::move(textbox));
	}

	std::span<const Textbox> get_textboxes() {
		return _textboxes;
	}

	std::span<const Textbox> get_textboxes(std::string_view path) {
		const auto [first, last] = std::equal_range(_textboxes.begin(), _textboxes.end(), Textbox{ path },
			[](const Textbox& a, const Textbox& b) {
				const size_t size = std::min(a.path.size(), b.path.size());
				return strncmp(a.path.data(), b.path.data(), size) < 0;
			});
		return { first, last };
	}

	// Returns true if the character at pos is plain text, defined as a character that is not part of a <...> tag.
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

	// Replaces graphical plain text with no-break spaces, starting at offset.
	// This is used to prevent the text from jumping around when being typed out.
	std::string _replace_graphical_plain_with_nbsp(std::string_view string, size_t offset) {
		constexpr char NBSP[] = { 0xC2, 0xA0 }; // UTF-8
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

	std::optional<Textbox> _textbox;
	float _typing_time = 0.f; // time since last character was typed
	size_t _typing_counter = 0; // number of characters typed

	const Textbox* get_current_textbox() {
		if (_textbox.has_value())
			return &*_textbox;
		return nullptr;
	}

	std::string _text;

	std::string_view get_current_typed_text() {
		return _text;
	}

	bool is_open() {
		return _textbox.has_value();
	}

	bool is_typing() {
		if (!_textbox) return false;
		return _typing_counter < _get_plain_count(_textbox->text);
	}

	void skip_typing() {
		if (!_textbox) return;
		_typing_counter = _get_plain_count(_textbox->text);
	}

	void open_now(const Textbox& textbox) {
		_textbox = textbox;
		_typing_time = 0.f;
		_typing_counter = 0;
		if (!_textbox->opening_sound.empty()) {
			audio::create_event(_textbox->opening_sound);
		}
	}

	std::deque<Textbox> _queue;

	void open_later(const Textbox& textbox) {
		_queue.push_back(textbox);
	}

	void open_next(const Textbox& textbox) {
		if (is_open()) {
			open_later(textbox);
		} else {
			open_now(textbox);
		}
	}

	void open_next(std::string_view path) {
		for (const Textbox& textbox : get_textboxes(path)) {
			open_next(textbox);
		}
	}

	void close_now() {
		_textbox.reset();
		_text.clear();
	}

	void close_all() {
		close_now();
		_queue.clear();
	}

	bool proceed() {
		if (_queue.empty()) {
			close_now();
			return false;
		}
		open_now(_queue.front());
		_queue.pop_front();
		return true;
	}

	void _create_textboxes(); // ui_textbox_creation.cpp

	void startup() {
		_textboxes.clear();
		_create_textboxes();
		std::sort(_textboxes.begin(), _textboxes.end(), [](const Textbox& a, const Textbox& b) {
			return a.path < b.path; });
	}

	size_t _selected_option = SIZE_MAX;

	void _on_pressed_confirm() {
		if (is_typing()) {
			skip_typing();
		} else if (_textbox->on_option_selected && _selected_option < _textbox->options.size()) {
			const std::string_view option = _textbox->options[_selected_option];
			_textbox->on_option_selected(option);
			audio::create_event("event:/ui/snd_button_click");
		} else {
			proceed();
		}
	}

	void _on_pressed_up() {
		if (_selected_option > 0) {
			_selected_option--;
			audio::create_event("event:/ui/snd_button_hover");
		}
	}

	void _on_pressed_down() {
		if (_selected_option + 1 < _textbox->options.size()) {
			_selected_option++;
			audio::create_event("event:/ui/snd_button_hover");
		}
	}

	void update(float dt) {

		if (!_textbox) return;

		if (input::pressed(window::Key::C)) {
			// TODO: If interaction opens a textbox, this immediately closes it!
			_on_pressed_confirm();
		}
		if (input::pressed(window::Key::Up)) {
			_on_pressed_up();
		}
		if (input::pressed(window::Key::Down)) {
			_on_pressed_down();
		}

		if (!_textbox) return;

		const size_t plain_count = _get_plain_count(_textbox->text);
		if (_typing_counter < plain_count && _textbox->typing_speed > 0.f) {
			float seconds_per_char = 1.f / _textbox->typing_speed;
			_typing_time += dt;
			if (_typing_time >= seconds_per_char) {
				_typing_time -= seconds_per_char;
				if (isgraph(_get_nth_plain(_textbox->text, _typing_counter))) {
					audio::create_event( _textbox->typing_sound);
				}
				++_typing_counter;
			}
		} else {
			_typing_counter = plain_count;
		}

		_text = _replace_graphical_plain_with_nbsp(_textbox->text, _typing_counter);
	}
}
}