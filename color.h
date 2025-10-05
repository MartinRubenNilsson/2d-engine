#pragma once

struct Color {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 255;
};

namespace colors {
    inline constexpr Color TRANSPARENT = { 0,     0,   0,   0 };
    inline constexpr Color BLACK       = { 0,     0,   0, 255 };
    inline constexpr Color WHITE       = { 255, 255, 255, 255 };
    inline constexpr Color RED         = { 255,   0,   0, 255 };
    inline constexpr Color GREEN       = { 0,   255,   0, 255 };
    inline constexpr Color BLUE        = { 0,     0, 255, 255 };
    inline constexpr Color YELLOW      = { 255, 255,   0, 255 };
    inline constexpr Color MAGENTA     = { 255,   0, 255, 255 };
    inline constexpr Color CYAN        = { 0,   255, 255, 255 };
}