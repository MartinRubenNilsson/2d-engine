#pragma once

namespace ui {
namespace textbox {
	enum class TextboxSprite {
		None,
		Skull,
		GoldenKey,
	};

	const char* get_sprite_name(TextboxSprite sprite);

	struct Textbox {
		static const std::string OPENING_SOUND_ITEM_FANFARE;
		static const std::string DEFAULT_TYPING_SOUND;

		std::string path;
		std::string text; // RML
		TextboxSprite sprite = TextboxSprite::None;
		std::string opening_sound; // name of sound event
		std::string typing_sound = DEFAULT_TYPING_SOUND;
		float typing_speed = 25.f; // in chars per second, 0 = instant
		std::vector<std::string> options;
		void (*options_callback)(const std::string& option) = nullptr;
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
	std::span<const Textbox> get_presets(const std::string& path);
	void open_or_enqueue_presets(const std::string& path);

	// DEBUGGING

	void show_debug_window();
}
}
