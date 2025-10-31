#include "stdafx.h"
#include "editor.h"
#include "editor_entities.h"

namespace editor {
	bool show = false;

	void _show_toolbar_imgui() {

	}

	void show_imgui() {
		// TODO: toolbar
		if (entities::show) {
			entities::show_imgui();
		}
	}
}