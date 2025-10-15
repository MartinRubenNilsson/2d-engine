#pragma once
#ifdef _DEBUG_IMGUI

namespace imgui_impl {
	bool startup();
	void shutdown();
	void new_frame();
	void render();
}

#endif // _DEBUG_IMGUI