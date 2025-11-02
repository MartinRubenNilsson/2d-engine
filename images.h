#pragma once

namespace images {
	struct Image {
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int channels = 0;
		void* data = nullptr;
		void* _private = nullptr; // For internal use only
	};

	bool load_image(const std::string& path, Image& image);
	void free_image(Image& image);
}