#include "stdafx.h"
#ifdef PLATFORM_WINDOWS
#include "platform.h"
#include "platform_debugging.h"
#include "platform_libraries.h"
#include "platform_directory_changes.h"
#include "console.h"
#include <Windows.h>
#include <shellapi.h>
#include <debugapi.h>
#include <libloaderapi.h>

extern int main(int argc, char* argv[]);

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	return main(__argc, __argv);
}

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

	struct DirectoryChangeContext {
		std::wstring directory_path;
		bool watch_subdirectories = false;
		HANDLE file{};
		std::unique_ptr<OVERLAPPED> overlapped{}; // needs to have stable memory addres
		std::vector<uint8_t> buffer;
	};

	std::vector<DirectoryChangeContext> _directory_change_contexts;

	bool _read_directory_changes(DirectoryChangeContext& context) {
		return ReadDirectoryChangesW(
			context.file,
			context.buffer.data(),
			(DWORD)context.buffer.size(),
			context.watch_subdirectories,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			nullptr,
			context.overlapped.get(),
			nullptr);
	}

	void _clear(DirectoryChangeContext& context) {
		CloseHandle(context.file);
		CloseHandle(context.overlapped->hEvent);
		context = {};
	}

	void start_watching_directory_changes(const std::wstring& directory_path, bool watch_subdirectories) {
		// Check if we're already watching this directory. If we are, and we're watching it in the same way,
		// do nothing. Otherwise if we want to watch it in another way, start watching anew.
		for (auto it = _directory_change_contexts.begin(); it != _directory_change_contexts.end(); it++) {
			if (it->directory_path != directory_path)
				continue; // This is not the directory we want to watch.
			if (it->watch_subdirectories == watch_subdirectories)
				return; // We're already watching this directory in the same way.
			_clear(*it);
			_directory_change_contexts.erase(it);
			break;
		}
		// PITFALL: Contrary to the name of the function, this just gets a handle to an existing directory.
		const HANDLE file = CreateFileW(directory_path.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr);
		if (file == INVALID_HANDLE_VALUE) {
			console::log_error("Error 5479629368: Cannot start watching directory changes, failed to open directory.");
			return;
		}
		DirectoryChangeContext context;
		context.directory_path = directory_path;
		context.watch_subdirectories = watch_subdirectories;
		context.file = file;
		context.overlapped = std::make_unique<OVERLAPPED>();
		context.overlapped->hEvent = CreateEvent(NULL, FALSE, 0, NULL);
		context.buffer.resize(1024);
		if (!_read_directory_changes(context)) {
			console::log_error("Error 1251671700: Cannot start watching directory changes, failed to read directory changes.");
			_clear(context);
			return;
		}
		_directory_change_contexts.emplace_back(std::move(context));
	}

	void stop_watching_directory_changes(const std::wstring& directory_path) {
		for (auto it = _directory_change_contexts.begin(); it != _directory_change_contexts.end(); it++) {
			if (it->directory_path != directory_path) continue;
			_clear(*it);
			it = _directory_change_contexts.erase(it);
		}
	}

	FileAction _get_action(const FILE_NOTIFY_INFORMATION& info) {
		switch (info.Action) {
			case FILE_ACTION_ADDED:            return FileAction::Added;
			case FILE_ACTION_REMOVED:          return FileAction::Removed;
			case FILE_ACTION_MODIFIED:         return FileAction::Modified;
			case FILE_ACTION_RENAMED_OLD_NAME: return FileAction::RenamedOldName;
			case FILE_ACTION_RENAMED_NEW_NAME: return FileAction::RenamedNewName;
			default:                           return FileAction::Added; // Should never happen.
		}
	}

	std::wstring _get_file_path(const FILE_NOTIFY_INFORMATION& info) {
		return { info.FileName, info.FileNameLength / sizeof(wchar_t) };
	}

	const FILE_NOTIFY_INFORMATION* _get_next(const FILE_NOTIFY_INFORMATION* info) {
		if (info->NextEntryOffset == 0) // if this is the last entry
			return nullptr;
		const uint8_t* bytes = (const uint8_t*)info;
		bytes += info->NextEntryOffset;
		return (const FILE_NOTIFY_INFORMATION*)bytes;
	}

	std::vector<DirectoryChange> _directory_changes;

	void update_directory_changes() {
		_directory_changes.clear();
		for (DirectoryChangeContext& context : _directory_change_contexts) {
			const DWORD result = WaitForSingleObject(context.overlapped->hEvent, 0);
			if (result != WAIT_OBJECT_0)
				continue; // The directory hasn't changed.
			DWORD num_bytes_transferred = 0;
			GetOverlappedResult(context.file, context.overlapped.get(), &num_bytes_transferred, FALSE);
			for (auto info = (const FILE_NOTIFY_INFORMATION*)context.buffer.data(); info; info = _get_next(info)) {
				std::wstring file_path = _get_file_path(*info);
				const FileAction action = _get_action(*info);
				_directory_changes.emplace_back(context.directory_path, std::move(file_path), action);
			}
			_read_directory_changes(context); // Start watching again.
		}
	}

	std::span<const DirectoryChange> get_directory_changes() {
		return _directory_changes;
	}

	void _clear_directory_change_contexts() {
		for (DirectoryChangeContext& context : _directory_change_contexts) {
			_clear(context);
		}
		_directory_change_contexts.clear();
	}

	void shutdown() {
		_clear_directory_change_contexts();
	}
}

#endif // PLATFORM_WINDOWS