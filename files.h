#pragma once

namespace files {
	enum class FileFormat {
		Unknown,
		Text, // .txt
		PngImage, // .png
		KhronosTexture, // .ktx2
		TiledWorld, // .world
		TiledMap, // .tmx
		TiledTileset, // .tsx
		TiledTemplate, // .tx
		TrueTypeFont, // .ttf
		GLSLVertexShader, // .vert
		GLSLFragmentShader, // .frag
		RmlUiDocument, // .rml
		RmlUiStyleSheet, // .rcss
		FmodStudioBank, // .bank
	};

	struct File {
		std::string path;
		FileFormat format = FileFormat::Unknown;
	};

	void startup();

	size_t get_file_count();
	std::span<const File> get_all_files();
	std::span<const File> get_all_files_in_directory(std::string_view directory_path); // recursive
	bool file_exists(std::string_view path);

	bool read_text_file(std::string_view path, std::string& text);
	bool write_text_file(std::string_view path, std::string_view text);
	bool read_binary_file(std::string_view path, std::vector<uint8_t>& data);
	bool write_binary_file(std::string_view path, std::span<const uint8_t> data);

	std::string get_normalized_path(std::string_view path);
	std::string get_parent_path(std::string_view path);
	std::string get_filename(std::string_view path);
	std::string get_stem(std::string_view path);
	std::string get_extension(std::string_view path);
	std::string replace_extension(std::string_view path, std::string_view new_extension);
}