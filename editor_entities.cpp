#include "stdafx.h"
#include "editor_entities.h"

namespace editor {
namespace entities {
	extern bool show = true;

	void show_imgui() {
		ImGui::Begin("Entities", &show);
		ImGui::End();
	}
}
}