#pragma once

#include <cstdint>


class Point {
public:
    uint16_t x;
    uint16_t y;
};

static_assert(sizeof(Point) == 4);

class Rect {
public:

    static Rect WH(uint16_t width, uint16_t height) { 
        return Rect{0,0,width, height};
    }
    static Rect XYWH(uint16_t left, uint16_t top, uint16_t width, uint16_t height) {
        return Rect{left, top, static_cast<uint16_t>(left + width), static_cast<uint16_t>(top + height)};
    }

    union {
        struct {
            Point topLeft;
            Point bottomRight;
        };
        struct {
            uint16_t left;
            uint16_t top;
            uint16_t right;
            uint16_t bottom;
        };
    };

    uint16_t width() const { return right - left; }
    uint16_t height() const { return bottom - top; }

private:
    Rect(uint16_t l, uint16_t t, uint16_t r, uint16_t b):
        left{l}, 
        top{t}, 
        right{r}, 
        bottom{b} {
    }

};

static_assert(sizeof(Rect) == 8);

/** A single pixel. 
 */
class Pixel {
public:

    constexpr Pixel(uint8_t r, uint8_t g, uint8_t b):
        raw_{static_cast<uint16_t>((r << 11) | (g << 6) | b)} {
        if (raw_ & 0b0000010000000000)
            raw_ |= 0b0000000000100000;
    }

    constexpr Pixel(uint16_t raw):
        raw_{raw} {
    }

    uint8_t r() const { return (raw_ >> 11) & 0b11111; }
    uint8_t g() const { return (raw_ >> 6) & 0b11111; }
    uint8_t b() const { return raw_ & 0b11111; }

    Pixel & r(uint8_t value) {
        raw_ &= 0b0000011111111111;
        raw_ |= value << 11;
        return *this;
    }

    Pixel & g(uint8_t value) {
        raw_ &= 0b1111100000011111;
        raw_ |= value << 6;
        if (raw_ & 0b0000010000000000)
            raw_ |= 0b0000000000100000;
        return *this;
    }

    Pixel & b(uint8_t value) {
        raw_ &= 0b1111111111100000;
        raw_ |= value;
        return *this;
    }

    /** Sets green color in its full range of 0..63. 
     */
    Pixel & gFull(uint8_t value) {
        raw_ &= 0b1111100000011111;
        raw_ |= value << 5;
        return *this;
    }

    operator uint16_t() const {
        return raw_;
    }

private:
    uint16_t raw_;
};

static_assert(sizeof(Pixel) == 2, "Wrong pixel size!");
