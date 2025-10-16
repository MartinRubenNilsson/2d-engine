#pragma once

namespace renderdoc {
	bool startup();
	bool is_frame_capturing();
	void open_capture_directory_if_frame_capturing();
}