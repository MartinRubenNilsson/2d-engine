#pragma once
#ifdef _DEBUG_RENDERDOC

namespace renderdoc {
	bool startup();
	bool is_frame_capturing();
	const char* get_capture_file_path_template();
}

#endif // DEBUG_RENDERDOC