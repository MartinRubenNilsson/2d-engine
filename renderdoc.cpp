#include "stdafx.h"
#ifdef _DEBUG_RENDERDOC
#include "renderdoc.h"
#include "files.h"
#include "console.h"
#include "platform.h"
#include <renderdoc/renderdoc_app.h>

namespace renderdoc {
	RENDERDOC_API_1_6_0* _api = nullptr;

	bool startup() {
		platform::Library lib = platform::load_library("renderdoc.dll");
		if (!lib.ptr) {
			console::log_error("Failed to load renderdoc.dll");
			return false;
		}
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)platform::get_library_proc(lib, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&_api);
		if (ret != 1) {
			console::log_error("Failed to get renderdoc API");
			return false;
		}
		//_rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
		//_rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);
		RENDERDOC_InputButton capture_key = eRENDERDOC_Key_F11;
		_api->SetCaptureKeys(&capture_key, 1);
		_api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
		return true;
	}

	bool is_frame_capturing() {
		if (!_api) return false;
		return _api->IsFrameCapturing();
	}

	void open_capture_directory_if_frame_capturing() {
		if (!_api) return;
		if (!_api->IsFrameCapturing()) return;
		// PITFALL: This is an UTF-8 string.
		const std::string_view path = _api->GetCaptureFilePathTemplate(); 
		const std::string parent_path = files::get_parent_path(path);
		platform::open(parent_path.c_str());
	}
}

#else // _DEBUG_RENDERDOC

namespace renderdoc {
	bool startup() { return true; }
	bool is_frame_capturing() { return false; }
	void open_capture_directory_if_frame_capturing() {}
}

#endif // _DEBUG_RENDERDOC
