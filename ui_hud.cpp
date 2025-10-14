#include "stdafx.h"
#include "ui_hud.h"
#include "graphics.h"

namespace ui {
namespace hud {
	bool show = false;
	unsigned int max_health = 3;
	unsigned int health = 2;
	unsigned int rupees = 0;
	unsigned int arrows = 0;
	unsigned int bombs = 0;

	void startup() {
		// TODO: load heart texture
	}

	void shutdown() {
		// TODO: unload heart texture
	}

	void update(float dt) {
		// TODO: load heart texture
	}

	void _layout_heart(unsigned int index, bool filled) {
		// TODO: image
		CLAY(CLAY_IDI("HudHeart", index), { .layout = {
			.sizing = { .width = CLAY_SIZING_FIXED(16), .height = CLAY_SIZING_FIXED(16) } },
			.backgroundColor = colors::WHITE }) {
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