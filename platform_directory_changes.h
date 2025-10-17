#pragma once

namespace platform {
	void start_watching_directory_changes(const std::wstring& directory_path, bool watch_subdirectories = false);
	void stop_watching_directory_changes(const std::wstring& directory_path);
	void update_directory_changes();

	enum class FileAction {
		Added, // A file was added to the directory.
		Removed, // A file was removed from the directory.
		Modified, // A file was modified. This can be a change in the time stamp or attributes.
		RenamedOldName, // A file was renamed and this is the old name.
		RenamedNewName, // A file was renamed and this is the new name.
	};

	struct DirectoryChange {
		std::wstring directory_path;
		std::wstring file_path;
		FileAction action = FileAction::Added;
	};

	std::span<const DirectoryChange> get_directory_changes();
}