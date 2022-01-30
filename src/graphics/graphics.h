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

    Pixel():
        raw_{0} {
    }

    static constexpr Pixel RGB(uint8_t r, uint8_t g, uint8_t b) {
        g = (g >= 16) ? (g << 1) : ((g << 1) + 1);
        return RGBFull(r, g, b);
    }
    static constexpr Pixel RGBFull(uint8_t r, uint8_t g, uint8_t b) {
        return Pixel{
            ((r & 31) << 3) |
            ((g >> 3) & 7) | ((g & 7) << 13) |
            (b & 31) << 8
        };
    }

    static constexpr Pixel Black() { return Pixel::RGB(0,0,0); }
    static constexpr Pixel Red() { return Pixel::RGB(31, 0, 0); };
    static constexpr Pixel Green() { return Pixel::RGB(0, 31, 0); };
    static constexpr Pixel Blue() { return Pixel::RGB(0, 0, 31); };
    static constexpr Pixel White() { return Pixel::RGB(31, 31, 31); };

    uint8_t r() const { return (raw_ >> 8) & 31; }
    uint8_t g() const { return ((raw_ & 7) << 2) | (( raw_ >> 14) & 3); }
    uint8_t b() const { return (raw_ >> 3) & 31; }

    Pixel & r(uint8_t value) {
        raw_ &= ~RED_MASK;
        raw_ |= (value << 8) & RED_MASK;
        return *this;
    }

    Pixel & g(uint8_t value) {
        value = (value >= 16) ? (value << 1) : ((value << 1) + 1);
        return gFull(value);
    }

    Pixel & b(uint8_t value) {
        raw_ &= ~BLUE_MASK;
        raw_ |= (value << 3) & BLUE_MASK;
        return * this;
    }

    uint8_t gFull() const { return ((raw_ & 7) << 3) | (( raw_ >> 13) & 7); }

    Pixel & gFull(uint8_t value) {
        raw_ &= ~GREEN_MASK;
        raw_ |= ((value >> 3) & 7) | ((value & 7) << 13);
        return *this;
    }

    constexpr operator uint16_t () {
        return raw_;
    }

private:

    constexpr Pixel(int raw):
        raw_{static_cast<uint16_t>(raw)} {
    }

    // GGGRRRRRBBBBBGGG
    static constexpr uint16_t RED_MASK = 0b0001111100000000;
    static constexpr uint16_t GREEN_MASK = 0b1110000000000111;
    static constexpr uint16_t BLUE_MASK = 0b0000000011111000;
    uint16_t raw_;
};

static_assert(sizeof(Pixel) == 2, "Wrong pixel size!");


/// Font data stored PER GLYPH
struct GFXglyph {
  uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
  uint8_t width;         ///< Bitmap dimensions in pixels
  uint8_t height;        ///< Bitmap dimensions in pixels
  uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
  int8_t xOffset;        ///< X dist from cursor pos to UL corner
  int8_t yOffset;        ///< Y dist from cursor pos to UL corner
};

/// Data stored for FONT AS A WHOLE
struct GFXfont{
  uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
  GFXglyph *glyph;  ///< Glyph array
  uint16_t first;   ///< ASCII extents (first char)
  uint16_t last;    ///< ASCII extents (last char)
  uint8_t yAdvance; ///< Newline distance (y axis)
};




#include "canvas.h"