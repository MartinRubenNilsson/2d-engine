#pragma once
#include <string_view>

namespace ui {
namespace textbox {
	enum class TextboxSprite {
		None,
		Skull,
		GoldenKey,
	};

	std::string_view get_sprite_name(TextboxSprite sprite);

	struct Textbox {
		static const std::string_view OPENING_SOUND_ITEM_FANFARE;
		static const std::string_view DEFAULT_TYPING_SOUND;

		std::string_view path;
		std::string_view text; // RML
		TextboxSprite sprite = TextboxSprite::None;
		std::string_view opening_sound; // name of sound event
		std::string_view typing_sound = DEFAULT_TYPING_SOUND;
		float typing_speed = 25.f; // in chars per second, 0 = instant
		std::vector<std::string_view> options;
		void (*options_callback)(std::string_view option) = nullptr;
	};

	void add_event_listeners();
	void create_presets();
	void update(float dt);

	bool is_open();
	bool is_typing();
	void skip_typing();

	// In addition to the current textbox (which may or may not be closed/empty), we store a queue of textboxes.
	// This is useful for sequencing textboxes, e.g. for a conversation. The interface works as follows:
	// 
	// - open(): immediately opens a new textbox, closing the current one if necessary.
	// - enqueue(): appends a textbox to the queue without affecting the current textbox.
	// - open_or_enqueue(): opens the textbox immediately if there is no current textbox, otherwise enqueues it.
	// - open_next_in_queue(): opens the next textbox in the queue, if any.
	// - close(): closes the current textbox without affecting the queue.
	// - close_and_clear_queue(): closes the current textbox and clears the queue.

	void open(const Textbox& textbox);
	void enqueue(const Textbox& textbox);
	void open_or_enqueue(const Textbox& textbox);
	bool open_next_in_queue();
	void close();
	void close_and_clear_queue();

	// PRESETS

	// Returns a list of all textbox presets, sorted lexicographically by Textbox::path.
	std::span<const Textbox> get_presets();
	// Returns a list of all textbox presets whose path starts with the given path.
	std::span<const Textbox> get_presets(std::string_view path);
	void open_or_enqueue_presets(std::string_view path);

	// DEBUGGING

	void show_debug_window();
}
}
