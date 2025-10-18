#pragma once

namespace platform {
	class DirectoryWatcher;

	Handle<DirectoryWatcher> watch_directory(std::wstring_view directory_path, bool watch_subdirectories = false);
	void stop_watching_directory(Handle<DirectoryWatcher> watcher);
	void update_directory_watchers();
	void shutdown_directory_watchers();

	enum class FileAction {
		Added, // A file was added to the directory.
		Removed, // A file was removed from the directory.
		Modified, // A file was modified. This can be a change in the time stamp or attributes.
		RenamedOldName, // A file was renamed and this is the old name.
		RenamedNewName, // A file was renamed and this is the new name.
	};

	struct DirectoryChange {
		FileAction action = FileAction::Added;
		std::wstring file_path;
	};

	// Returns all new changes since the last call to update_directory_watchers().
	std::span<const DirectoryChange> get_directory_changes(Handle<DirectoryWatcher> watcher);
}