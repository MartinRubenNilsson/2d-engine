#include "stdafx.h"
#ifdef PLATFORM_WINDOWS
#include "platform.h"
#include "platform_debugging.h"
#include "platform_libraries.h"
#include "platform_directory_changes.h"
#include "console.h"
#include "pool.h"

#include <Windows.h>
#include <shellapi.h>
#include <debugapi.h>
#include <libloaderapi.h>

extern int main(int argc, char* argv[]);

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	return main(__argc, __argv);
}

namespace platform {
	int system(const std::string& command) {
		return ::system(command.c_str());
	}

	bool set_environment_variable(const std::string& name, const std::string& value) {
		return SetEnvironmentVariableA(name.c_str(), value.c_str());
	}

	bool _shell_execute_succeeded(HINSTANCE result) {
		// https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea#return-value
		return (INT_PTR)result > 32;
	}

	bool open(const std::string& path) {
		HINSTANCE result = ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		return _shell_execute_succeeded(result);
	}

	////////////////////////////
	// platform_debugging.cpp //
	////////////////////////////

	bool is_debugger_present() {
		return IsDebuggerPresent();
	}

	void debug_break() {
		DebugBreak();
	}

	void output_debug_string(const std::string& string) {
		OutputDebugStringA(string.c_str());
	}

	////////////////////////////
	// platform_libraries.cpp //
	////////////////////////////

	Library load_library(const std::string& lib_file_name) {
		return { .ptr = (uintptr_t)LoadLibraryA(lib_file_name.c_str()) };
	}

	LibraryProc get_library_proc(Library lib, const std::string& proc_name) {
		return GetProcAddress((HMODULE)lib.ptr, proc_name.c_str());
	}

	////////////////////////////////////
	// platform_directory_changes.cpp //
	////////////////////////////////////

	FileAction _get_file_action(const FILE_NOTIFY_INFORMATION& info) {
		switch (info.Action) {
			case FILE_ACTION_ADDED:            return FileAction::Added;
			case FILE_ACTION_REMOVED:          return FileAction::Removed;
			case FILE_ACTION_MODIFIED:         return FileAction::Modified;
			case FILE_ACTION_RENAMED_OLD_NAME: return FileAction::RenamedOldName;
			case FILE_ACTION_RENAMED_NEW_NAME: return FileAction::RenamedNewName;
			default:                           return FileAction::Added; // Should never happen.
		}
	}

	std::wstring _get_file_name(const FILE_NOTIFY_INFORMATION& info) {
		return { info.FileName, info.FileNameLength / sizeof(wchar_t) };
	}

	const FILE_NOTIFY_INFORMATION* _get_next(const FILE_NOTIFY_INFORMATION* info) {
		if (info->NextEntryOffset == 0) // if this is the last entry
			return nullptr;
		const uint8_t* bytes = (const uint8_t*)info;
		bytes += info->NextEntryOffset;
		return (const FILE_NOTIFY_INFORMATION*)bytes;
	}

	class DirectoryWatcher {
		std::wstring _directory_path;
		bool _watch_subdirectories = false;
		HANDLE _directory_file_handle = INVALID_HANDLE_VALUE;
		std::unique_ptr<OVERLAPPED> _overlapped{}; // PITFALL: needs to have stable memory address!
		std::vector<uint8_t> _buffer; // for storing FILE_NOTIFY_INFORMATION:s in
		std::vector<DirectoryChange> _changes;

	public:
		DirectoryWatcher(std::wstring_view directory_path, bool watch_subdirectories)
			: _directory_path(directory_path)
			, _watch_subdirectories(watch_subdirectories)
			, _overlapped(std::make_unique<OVERLAPPED>())
			, _buffer(1024)
		{
			_overlapped->hEvent = CreateEvent(NULL, FALSE, 0, NULL);
		}

		DirectoryWatcher(const DirectoryWatcher&) = delete;
		DirectoryWatcher& operator=(const DirectoryWatcher&) = delete;

		DirectoryWatcher(DirectoryWatcher&& other) noexcept
			: _directory_path(std::move(other._directory_path))
			, _watch_subdirectories(other._watch_subdirectories)
			, _directory_file_handle(other._directory_file_handle)
			, _overlapped(std::move(other._overlapped))
			, _buffer(std::move(other._buffer))
			, _changes(std::move(other._changes))
		{
			other._directory_file_handle = INVALID_HANDLE_VALUE;
		}

		DirectoryWatcher& operator=(DirectoryWatcher&& other) noexcept {
			_directory_path = std::move(other._directory_path);
			_watch_subdirectories = other._watch_subdirectories;
			_directory_file_handle = other._directory_file_handle;
			other._directory_file_handle = INVALID_HANDLE_VALUE;
			_overlapped = std::move(other._overlapped);
			_buffer = std::move(other._buffer);
			_changes = std::move(other._changes);
			return *this;
		}

		void clear() {
			_directory_path.clear();
			if (_directory_file_handle != INVALID_HANDLE_VALUE) {
				CloseHandle(_directory_file_handle);
				_directory_file_handle = INVALID_HANDLE_VALUE;
			}
			_watch_subdirectories = false;
			if (_overlapped && _overlapped->hEvent) {
				CloseHandle(_overlapped->hEvent);
				_overlapped->hEvent = nullptr;
			}
			_overlapped.reset();
			_buffer.clear();
			_changes.clear();
		}

		~DirectoryWatcher() {
			clear();
		}

		bool open_directory() {
			if (_directory_path.empty())
				return false;
			if (_directory_file_handle != INVALID_HANDLE_VALUE) {
				CloseHandle(_directory_file_handle); // DEFENSIVE: in case the handle is already open
				_directory_file_handle = INVALID_HANDLE_VALUE;
			}
			_directory_file_handle = CreateFileW(
				_directory_path.c_str(),
				FILE_LIST_DIRECTORY,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				nullptr,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
				nullptr);
			return _directory_file_handle != INVALID_HANDLE_VALUE;
		}

		bool start_watching() {
			if (_directory_file_handle == INVALID_HANDLE_VALUE)
				return false;
			if (_buffer.empty())
				return false;
			if (!_overlapped)
				return false;
			return ReadDirectoryChangesW(
				_directory_file_handle,
				_buffer.data(),
				(DWORD)_buffer.size(),
				_watch_subdirectories,
				FILE_NOTIFY_CHANGE_FILE_NAME |
				FILE_NOTIFY_CHANGE_DIR_NAME |
				FILE_NOTIFY_CHANGE_LAST_WRITE,
				nullptr,
				_overlapped.get(),
				nullptr);
		}

		void clear_changes() {
			_changes.clear();
		}

		bool has_new_changes() const {
			if (!_overlapped) return false;
			if (!_overlapped->hEvent) return false;
			const DWORD ret = WaitForSingleObject(_overlapped->hEvent, 0);
			return ret == WAIT_OBJECT_0;
		}

		// Returns how many new changes were added.
		size_t append_new_changes() {
			if (_directory_file_handle == INVALID_HANDLE_VALUE)
				return 0;
			if (!_overlapped)
				return 0;
			if (_buffer.empty())
				return 0;
			DWORD num_bytes_transferred = 0;
			GetOverlappedResult(_directory_file_handle, _overlapped.get(), &num_bytes_transferred, FALSE);
			const size_t num_changes_before = _changes.size();
			for (auto info = (const FILE_NOTIFY_INFORMATION*)_buffer.data(); info; info = _get_next(info)) {
				const FileAction action = _get_file_action(*info);
				std::wstring file_path = _get_file_name(*info);
				_changes.emplace_back(action, std::move(file_path));
			}
			return _changes.size() - num_changes_before;
		}

		std::span<const DirectoryChange> get_changes() const {
			return _changes;
		}
	};

	Pool<DirectoryWatcher> _directory_watchers;

	Handle<DirectoryWatcher> watch_directory(std::wstring_view directory_path, bool watch_subdirectories) {
		DirectoryWatcher watcher(directory_path, watch_subdirectories);
		if (!watcher.open_directory()) {
			console::log_error("Error 5479629: Failed to watch directory, failed to open directory.");
			return {};
		}
		if (!watcher.start_watching()) {
			console::log_error("Error 1251671: Failed to watch directory, failed to start watching.");
			return {};
		}
		return _directory_watchers.emplace(std::move(watcher));
	}

	void stop_watching_directory(Handle<DirectoryWatcher> watcher) {
		// PITFALL: free() doesn't deconstruct or cleanup pool elements, so we need to do so ourselves.
		if (DirectoryWatcher* w = _directory_watchers.get(watcher)) {
			w->clear();
		}
		_directory_watchers.free(watcher);
	}

	void update_directory_watchers() {
		for (DirectoryWatcher& watcher : _directory_watchers.span()) {
			// PITFALL: watcher may be invalid here, but I've written the clear_changes() and
			// has_new_changes() to be able to handle this (the latter should return false).
			watcher.clear_changes();
			if (!watcher.has_new_changes())
				continue;
			watcher.append_new_changes();
			watcher.start_watching(); // Necessary to keep watching.
		}
	}

	void shutdown_directory_watchers() {
		_directory_watchers.clear();
	}

	std::span<const DirectoryChange> get_directory_changes(Handle<DirectoryWatcher> watcher) {
		DirectoryWatcher* w = _directory_watchers.get(watcher);
		if (!w) return {};
		return w->get_changes();
	}
}

#endif // PLATFORM_WINDOWS