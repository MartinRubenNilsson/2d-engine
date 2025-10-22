#pragma once
#include <string_view>

namespace ui {
namespace textbox {
	enum class TextboxSprite { // TODO: remove
		None,
		Skull,
		GoldenKey,
	};

	std::string_view get_sprite_name(TextboxSprite sprite); // TODO: remove

	extern const std::string_view DEFAULT_TYPING_SOUND;

	struct Textbox {
		std::string_view path;
		std::string_view text;
		TextboxSprite sprite = TextboxSprite::None;
		std::string_view opening_sound; // name of sound event
		std::string_view typing_sound = DEFAULT_TYPING_SOUND;
		float typing_speed = 25.f; // in chars per second, 0 = instant
		std::vector<std::string_view> options;
		void (*options_callback)(std::string_view option) = nullptr;
	};

	// Creates a textbox, i.e. adds it to an internal list of registered textbox presets.
	void create_textbox(Textbox&& textbox);
	// Returns a sorted list of all textboxes, sorted lexicographically by Textbox::path.
	std::span<const Textbox> get_textboxes();
	// Returns a sorted list of all textbox whose path starts with the given path.
	std::span<const Textbox> get_textboxes(std::string_view path);

	bool is_open();
	bool is_typing();
	void skip_typing();

	// In addition to a current textbox (which may or may not be closed/empty), we store a queue of textboxes.
	// This is useful for sequencing textboxes, e.g. for a conversation. The API works as follows:
	// 
	// - open_now(): Immediately opens a new textbox, closing any currently open textbox.
	// - open_later(): Appends a textbox to the end of the queue without affecting the current textbox.
	// - open_next(): Opens the textbox immediately if there is no current textbox open, otherwise enqueues it.
	// - close_now(): Closes the current textbox without affecting the queue.
	// - close_all(): Closes the current textbox and clears the queue.
	// - proceed(): Closes the current textbox (if any) and opens the next textbox in the queue (if any).

	void open_now(const Textbox& textbox);
	void open_later(const Textbox& textbox);
	void open_next(const Textbox& textbox);
	void open_next(std::string_view path);
	void close_now();
	void close_all();
	bool proceed(); // Returns true if there was a next textbox to open.

	void startup();
	void update(float dt);
}
}
