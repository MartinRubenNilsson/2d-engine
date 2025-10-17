#include "stdafx.h"
#include "ui_hud.h"
#include "ui_clay.h"
#include "graphics.h"
#include "text_fonts.h"

namespace ui {
namespace hud {
	bool show = false;
	unsigned int max_health = 3;
	unsigned int health = 2;
	unsigned int rupees = 0;
	unsigned int arrows = 0;
	unsigned int bombs = 0;

	text::FontId _font{};
	Handle<graphics::Texture> _spritesheet{};
	Image _heart_image{};

	void startup() {
		_font = text::load_font("assets/fonts/Retro Gaming.ttf");
		_spritesheet = graphics::load_texture("assets/ui/hud.png");
		_heart_image.texture = _spritesheet;
		_heart_image.tex_rect_pos = { 16, 16 };
		_heart_image.tex_rect_size = { 16, 16 };
	}

	std::string _health_string;
	std::string _rupees_string;
	std::string _arrows_string;
	std::string _bombs_string;

	void update(float dt) {
		_health_string = "Health: " + std::to_string(health);
	}

	void _layout_heart(unsigned int index, bool filled) {
		CLAY(CLAY_IDI("HudHeart", index), { .layout = {
			.sizing = {
				.width = CLAY_SIZING_FIXED(16),
				.height = CLAY_SIZING_FIXED(16) } },
			.image = {
				.imageData = &_heart_image } })
		{}
	}

	void _layout_health() {
		CLAY(CLAY_ID("HudHealth"), { .layout = {
			.childGap = 3 } })
		{
			for (unsigned int i = 0; i < max_health; ++i) {
				_layout_heart(i, i < health);
			}
		}
	}

	Clay_String _to_clay_string(std::string_view string) {
		return { false, (int32_t)string.size(), string.data() };
	}

	void _layout_text(std::string_view string) {
		CLAY_TEXT(_to_clay_string(string), CLAY_TEXT_CONFIG({ .fontId = _font.id, .fontSize = 8 }));
	}

	void layout() {
		CLAY(CLAY_ID("HudContainer"), { .layout = {
			.padding = CLAY_PADDING_ALL(3),
			.childGap = 3,
			.layoutDirection = CLAY_TOP_TO_BOTTOM } })
		{
			_layout_health();
			_layout_text(_health_string);
			//_layout_text("Arrows: " + std::to_string(arrows));
			//_layout_text("Bombs: " + std::to_string(bombs));
		}
	}
}
	/// OLD

	extern Rml::Context* _context;

	Rml::ElementDocument* _get_hud_document() {
		return _context->GetDocument("hud");
	}

	bool get_hud_visible() {
		Rml::ElementDocument* doc = _get_hud_document();
		return doc && doc->IsVisible();
	}

	void set_hud_visible(bool visible) {
		if (Rml::ElementDocument* doc = _get_hud_document()) {
			visible ? doc->Show() : doc->Hide();
		}
	}
}