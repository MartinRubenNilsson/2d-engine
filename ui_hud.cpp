#include "stdafx.h"
#include "ui_hud.h"
#include "ui_clay.h"
#include "graphics.h"

namespace ui {
namespace hud {
	bool show = false;
	unsigned int max_health = 3;
	unsigned int health = 2;
	unsigned int rupees = 0;
	unsigned int arrows = 0;
	unsigned int bombs = 0;

	Handle<graphics::Texture> _spritesheet{};
	Image _heart_image{};

	void startup() {
		_spritesheet = graphics::load_texture("assets/ui/hud.png");
		_heart_image.texture = _spritesheet;
		_heart_image.tex_rect_pos = { 16, 16 };
		_heart_image.tex_rect_size = { 16, 16 };
	}

	void shutdown() {
	}

	void update(float dt) {
	}

	void _layout_heart(unsigned int index, bool filled) {
		CLAY(CLAY_IDI("HudHeart", index), { .layout = {
			.sizing = { .width = CLAY_SIZING_FIXED(16), .height = CLAY_SIZING_FIXED(16) } },
			.image = { .imageData = &_heart_image } }) {
		}
	}

	void _layout_health() {
		CLAY(CLAY_ID("HudHealth"), { .layout = { .childGap = 3 } }) {
			for (unsigned int i = 0; i < max_health; ++i) {
				_layout_heart(i, i < health);
			}
		}
	}

	void layout() {
		CLAY(CLAY_ID("HudContainer"), { .layout = { .padding = CLAY_PADDING_ALL(3), .childGap = 3 } }) {
			_layout_health();
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