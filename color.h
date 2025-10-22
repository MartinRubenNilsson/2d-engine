#pragma once

struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : r(r), g(g), b(b), a(a) {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b)
        : Color(r, g, b, 255) {}
    constexpr Color(const Clay_Color& c)
        : Color((uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b, (uint8_t)c.a) {}

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
    static const Color SILVER;
    static const Color GRAY;
    static const Color DIM_GRAY;
};
