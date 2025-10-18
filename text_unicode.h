#pragma once

namespace text {
	// Converts the next (multibyte) UTF-8 Unicode codepoint to its UTF-32 counterpart.
	// Shrinks the beginning of string by the number of bytes consumed. Returns the null
	// character if string is empty or if the next character couldn't be decoded.
	char32_t to_c32(std::u8string_view& string);

	// Returns the length in Unicode codepoints, which is in general less than string.size().
	size_t length(std::u8string_view string);
}