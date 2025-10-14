#pragma once

struct Color {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 255;

    constexpr operator Clay_Color() const {
        return { (float)r, (float)g, (float)b, (float)a };
    }

    constexpr auto operator<=>(const Color&) const = default;

    static const Color TRANSPARENT;
    static const Color BLACK;
    static const Color WHITE;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color MAGENTA;
    static const Color CYAN;
};
