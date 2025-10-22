#include "stdafx.h"
#include "ui_textbox.h"
#include "ui_bindings.h"
#include "input.h"
#include "audio.h"
#include <deque>

namespace ui {
	namespace bindings {
		void _clear_bindings() {
			textbox_text.clear();
			textbox_has_sprite = false;
			textbox_sprite.clear();
			textbox_has_options = false;
			textbox_options.clear();
			textbox_selected_option = 0;
		}
	}

	extern Rml::Context* _context;

namespace textbox {

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

	// Returns true if the character at pos is plain text,
	// defined as a character that is not part of an RML tag.
	bool _is_plain(std::string_view rml, size_t pos) {
		for (size_t i = pos; i < rml.size(); ++i) {
			if (rml[i] == '<') return i != pos;
			if (rml[i] == '>') return false;
		}
		return true;
	}

	// Counts the number of plain text characters in the string.
	size_t _get_plain_count(std::string_view rml) {
		size_t count = 0;
		for (size_t i = 0; i < rml.size(); ++i) {
			if (_is_plain(rml, i)) {
				++count;
			}
		}
		return count;
	}

	// Returns the nth plain text character if n < _get_plain_length(rml), or '\0' otherwise.
	char _get_nth_plain(std::string_view rml, size_t n) {
		size_t count = 0;
		for (size_t i = 0; i < rml.size(); ++i) {
			if (_is_plain(rml, i)) {
				if (count == n) return rml[i];
				++count;
			}
		}
		return '\0';
	}

	// Replaces graphical plain text with non-breaking spaces, starting at offset.
	// This is used to prevent the text from jumping around when being typed out.
	std::string _replace_graphical_plain_with_nbsp(std::string_view rml, size_t offset) {
		std::string ret;
		size_t count = 0;
		for (size_t i = 0; i < rml.size(); ++i) {
			bool replace = false;
			if (_is_plain(rml, i)) {
				if (count >= offset && isgraph(rml[i])) {
					replace = true;
				}
				++count;
			}
			ret += replace ? "&nbsp;" : rml.substr(i, 1);
		}
		return ret;
	}

	Rml::ElementDocument* _get_document() {
		return _context->GetDocument("textbox");
	}

	void _set_document_visible(bool visible) {
		if (Rml::ElementDocument* doc = _get_document()) {
			visible ? doc->Show() : doc->Hide();
		}
	}

	std::optional<Textbox> _textbox;
	float _typing_time = 0.f; // time since last character was typed
	size_t _typing_counter = 0; // number of characters typed

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
			const std::string path(_textbox->opening_sound);
			audio::create_event({ .path = path.c_str() });
		}
		_set_document_visible(true);
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
		bindings::_clear_bindings();
		_set_document_visible(false);
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

	void _on_pressed_confirm() {
		if (is_typing()) {
			skip_typing();
		} else if (_textbox->options_callback &&
			bindings::textbox_selected_option < bindings::textbox_options.size()) {
			const std::string_view option = bindings::textbox_options[bindings::textbox_selected_option];
			_textbox->options_callback(option);
			audio::create_event({ .path = "event:/ui/snd_button_click" });
		} else {
			proceed();
		}
	}

	void _on_pressed_up() {
		if (bindings::textbox_selected_option > 0) {
			bindings::textbox_selected_option--;
			audio::create_event({ .path = "event:/ui/snd_button_hover" });
		}
	}

	void _on_pressed_down() {
		if (bindings::textbox_selected_option + 1 < bindings::textbox_options.size()) {
			bindings::textbox_selected_option++;
			audio::create_event({ .path = "event:/ui/snd_button_hover" });
		}
	}

	void update(float dt) {

		if (!_textbox) return;

		if (input::pressed(window::Key::C)) {
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
					const std::string path(_textbox->typing_sound);
					audio::create_event({ .path = path.c_str() });
				}
				++_typing_counter;
			}
		} else {
			_typing_counter = plain_count;
		}

		const bool finished_typing = (_typing_counter == plain_count);

		bindings::textbox_text = _replace_graphical_plain_with_nbsp(_textbox->text, _typing_counter);
		bindings::textbox_has_sprite = (_textbox->sprite != TextboxSprite::None);
		bindings::textbox_sprite = get_sprite_name(_textbox->sprite);
		if (finished_typing) {
			bindings::textbox_has_options = !_textbox->options.empty();
			bindings::textbox_options.resize(_textbox->options.size());
			for (size_t i = 0; i < _textbox->options.size(); ++i) {
				bindings::textbox_options[i] = _textbox->options[i];
			}
		} else {
			bindings::textbox_has_options = false;
			bindings::textbox_options.clear();
			bindings::textbox_selected_option = 0;
		}
	}
}
}