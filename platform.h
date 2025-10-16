#pragma once

namespace platform {
	int system(const char* command);
	bool set_environment_variable(const char* name, const char* value);
	bool open(const char* path);
	bool is_debugger_present();
	void debug_break();
	void output_debug_string(const char* string);

	// A handle to a dynamic-link library (DLL).
	struct Library {
		uintptr_t ptr = 0;
	};

	Library load_library(const char* lib_file_name);

	// An address to an exported library procedure (function).
	using LibraryProc = const void*;

	LibraryProc get_library_proc(Library lib, const char* proc_name);
}

#ifdef _DEBUG
#define _DEBUG_BREAK() platform::debug_break()
#define _OUTPUT_DEBUG_STRING(string) platform::output_debug_string(string)
#else
#define _DEBUG_BREAK()
#define _OUTPUT_DEBUG_STRING(string)
#endif