#include "stdafx.h"
#ifdef _DEBUG_RENDERDOC
#include "renderdoc.h"
#include "console.h"
#include <Windows.h>
#include <renderdoc/renderdoc_app.h>

namespace renderdoc {
	RENDERDOC_API_1_6_0* _rdoc_api = nullptr;

	bool initialize() {
		HMODULE mod = LoadLibraryA("renderdoc.dll");
		if (!mod) {
			console::log_error("Failed to load renderdoc.dll");
			return false;
		}
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&_rdoc_api);
		if (ret != 1) {
			console::log_error("Failed to get renderdoc API");
			return false;
		}
		//_rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
		//_rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);
		RENDERDOC_InputButton capture_key = eRENDERDOC_Key_F11;
		_rdoc_api->SetCaptureKeys(&capture_key, 1);
		_rdoc_api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
		return true;
	}

	bool is_frame_capturing() {
		if (!_rdoc_api) return false;
		return _rdoc_api->IsFrameCapturing();
	}

	const char* get_capture_file_path_template() {
		if (!_rdoc_api) return "";
		return _rdoc_api->GetCaptureFilePathTemplate();
	}
}

#endif // _DEBUG_RENDERDOC
