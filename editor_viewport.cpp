#include "stdafx.h"
#include "editor_viewport.h"

namespace editor {
namespace viewport {
	bool show = false;

	void show_imgui() {
		ImGui::Begin("Viewport");
		ImGui::End();
	}
}
}