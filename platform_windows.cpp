#include "stdafx.h"
#ifdef PLATFORM_WINDOWS
#include "platform.h"
#include <Windows.h>
#include <shellapi.h> // ShellExecuteA
#include <debugapi.h>
#include <libloaderapi.h>

namespace platform {
	int system(const char* command) {
		return ::system(command);
	}

	bool set_environment_variable(const char* name, const char* value) {
		return SetEnvironmentVariableA(name, value);
	}

	bool _shell_execute_succeeded(HINSTANCE result) {
		// https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea#return-value
		return (INT_PTR)result > 32;
	}

	bool open(const char* path) {
		HINSTANCE result = ShellExecuteA(nullptr, "open", path, nullptr, nullptr, SW_SHOWNORMAL);
		return _shell_execute_succeeded(result);
	}

	bool is_debugger_present() {
		return IsDebuggerPresent();
	}

	void debug_break() {
		DebugBreak();
	}

	void output_debug_string(const char* string) {
		OutputDebugStringA(string);
	}

	Library load_library(const char* lib_file_name) {
		return { .ptr = (uintptr_t)LoadLibraryA(lib_file_name) };
	}

	LibraryProc get_library_proc(Library lib, const char* proc_name) {
		return GetProcAddress((HMODULE)lib.ptr, proc_name);
	}
}

#endif // PLATFORM_WINDOWS