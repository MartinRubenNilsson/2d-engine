#pragma once

namespace platform {
	// A handle to a dynamic-link library (DLL).
	struct Library {
		uintptr_t ptr = 0;
	};

	Library load_library(const std::string& lib_file_name);

	// An address to an exported library procedure (function).
	using LibraryProc = const void*;

	LibraryProc get_library_proc(Library lib, const std::string& proc_name);
}