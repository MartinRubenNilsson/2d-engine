#include "stdafx.h"
#include "text_unicode.h"
#include <cuchar>

static_assert(__STDC_UTF_16__, "mbrtoc16() and c16rtomb() are not guaranteed to use UTF-16 encoding");
static_assert(__STDC_UTF_32__, "mbrtoc32() and c32rtomb() are not guaranteed to use UTF-32 encoding");

namespace text {
	enum class ConversionResult {
		Success, // converted a UTF-8 multibyte character to a single UTF-32 character
		Null, // succeeded, and the resulting UTF-32 character is the null character
		Error, // encoding error occured
		Incomplete, // encountered an incomplete (but so far valid) UTF-8 multibyte character
		Multibyte, // succeded, and the converted character is the continuatin of a UTF-32 multibyte character 
	};

	// Converts the next UTF-8 multibyte character in a string to its UTF-32 character representation.
	// Shrinks the beginning of the string with the number of UTF-8 characters consumed, so calling
	// this function repeatedly with the same (valid) string and state should fully convert the string.
	ConversionResult _to_c32(char32_t& c, std::u8string_view& string, std::mbstate_t& state) {
		const size_t ret = mbrtoc32(&c, (const char*)string.data(), string.size(), &state);
		switch (ret) {
			default:
				string.remove_prefix(ret);
				return ConversionResult::Success;
			case 0:
				return ConversionResult::Null;
			case static_cast<size_t>(-1):
				return ConversionResult::Error;
			case static_cast<size_t>(-2):
				return ConversionResult::Incomplete; // this is what you get if string is empty
			case static_cast<size_t>(-3):
				return ConversionResult::Multibyte;
		}
	}

	char32_t to_c32(std::u8string_view& string) {
		if (string.empty())
			return 0;
		char32_t c = 0;
		std::mbstate_t state{};
		ConversionResult result = _to_c32(c, string, state);
		if (result == ConversionResult::Success ||
			result == ConversionResult::Multibyte) {
			return c;
		}
		return 0;
	}

	size_t length(std::u8string_view string) {
		size_t len = 0;
		std::mbstate_t state{};
		while (!string.empty()) {
			char32_t c = 0;
			ConversionResult result = _to_c32(c, string, state);
			if (result == ConversionResult::Multibyte) {
				continue; // c is part of a multibyte UTF-32 character and must not be double-counted.
			}
			if (result == ConversionResult::Success) {
				len++;
				continue;
			}
			break; // Null, Error, Incomplete
		}
		return len;
	}
}