#include "stdafx.h"
#include "ui_textbox.h"
#include "ui_rmlui.h"

namespace ui
{
	const char* get_textbox_sprite_name(TextboxSprite sprite)
	{
		switch (sprite) {
		case TextboxSprite::None:      return "";
		case TextboxSprite::Skull:     return "icon-skull";
		case TextboxSprite::GoldenKey: return "icon-golden-key";
		default:                       return "";
		}
	}

	//TODO: replace with enum and getter
	const std::string Textbox::OPENING_SOUND_ITEM_FANFARE = "snd_item_fanfare";
	const std::string Textbox::DEFAULT_TYPING_SOUND = "snd_txt1";

	std::vector<Textbox> _textbox_presets;

	std::span<const Textbox> get_textbox_presets()
	{
		return _textbox_presets;
	}

	std::span<const Textbox> get_textbox_presets(const std::string& path)
	{
		const size_t path_size = path.size();
		auto [first, last] = std::equal_range(_textbox_presets.begin(), _textbox_presets.end(), Textbox{ path },
			[path_size](const Textbox& left, const Textbox& right) {
				return strncmp(left.path.c_str(), right.path.c_str(), path_size) < 0;
			});
		return { first, last };
	}

	void create_textbox_presets()
	{
		_textbox_presets.clear();

		{
			Textbox& tb = _textbox_presets.emplace_back();
			tb.path = "player/die/0";
			tb.text = "You are <span style='color: red'>deader than dead</span>!<br/>Oh, what a pity that your adventure should end here, and so soon...";
			tb.sprite = TextboxSprite::Skull;
		}
		{
			Textbox& tb = _textbox_presets.emplace_back();
			tb.path = "player/die/1";
			tb.text = "Would you like to try again?";
			tb.options = { "Yes", "No" };
			tb.options_callback = [](const std::string& option) {
				if (option == "Yes") {
					bindings::on_click_restart();
				} else if (option == "No") {
					bindings::on_click_main_menu();
				}
			};
		}

		std::sort(_textbox_presets.begin(), _textbox_presets.end(),
			[](const Textbox& left, const Textbox& right) { return left.path < right.path; });
	}
}