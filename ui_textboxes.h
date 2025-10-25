#pragma once

namespace ui {
	struct ImageData;

namespace textboxes {
	struct TextboxOption {
		std::string_view text;
		void (*on_selected)() = nullptr;
	};

	struct Textbox {
		// The path uniquely identifies the textbox and also groups related textboxes together,
		// much like you would put related files in the same directory. Example:
		// 
		//	 player/die/0
		//   player/die/1
		//   ...
		//
		// Note however that this is just a imaginary path (i.e. does not correspond to a file).
		//
		std::string_view path;
		std::string_view text; // UTF-8
		std::string_view opening_sound; // Audio event path.
		std::string_view typing_sound = "event:/snd_txt1"; // Audio event path.
		float typing_speed = 25.f; // In chars per second. Set to 0.f to reveal all text instantly.
		ImageData* image = nullptr;
		std::vector<TextboxOption> options;
	};

	// Adds a textbox to the list of textbox presets. This should only be called at startup.
	void add_textbox(Textbox&& textbox);
	// Returns a sorted list of all textbox presets, sorted by Textbox::path.
	std::span<const Textbox> get_textboxes();
	// Returns a sorted list of all textboxes whose path starts with the given path.
	std::span<const Textbox> get_textboxes_starting_with(std::string_view path);

	bool closed(); // Is the textbox currently closed?
	const Textbox& get_current_textbox(); // Returns the currenty open textbox. May be an empty textbox.
	std::string_view get_typed_text(); // Up to how much have been typed.
	bool is_typing_text(); // Is the text still being typed out?
	void skip_typing_text(); // Reveals all text.

	void set_current_option(size_t option);
	size_t get_current_option();
	void select_current_option();

	// In addition to the current textbox (which may or may not be open), we store a queue of textboxes.
	// This can be used to sequence them, e.g. for a dialogue. The API works as follows:
	// 
	// - close(): Closes the current textbox without affecting the queue.
	// - close_all(): Closes the current textbox and clears the queue.
	// - open(): Immediately opens a new textbox, replacing any currently open textbox.
	// - open_next(): Opens the textbox immediately if there is no textbox open, otherwise enqueues it/them.
	// - proceed(): Closes the current textbox (if any) and opens the next textbox in the queue (if any).

	void close();
	void close_all();
	void open(const Textbox& textbox);
	void open_next(const Textbox& textbox);
	void open_next(std::string_view path);
	void proceed();

	void startup();
	void update(float dt);
	void layout();
}
}
