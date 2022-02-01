#pragma once

#include <memory.h>
#include <algorithm>

#include <hardware/dma.h>

#include "graphics.h"


/** Canvas holds the pixel data and provides basic drawing operations. 

 */
class Canvas {
public:
    Canvas(uint16_t width, uint16_t height):
        width_{width},
        height_{height},
        buffer_{new Pixel[width * height]} {
    }

    ~Canvas() {
        delete [] buffer_;
    }

    Pixel fg() const {
        return fg_;
    }

    Pixel bg() const {
        return bg_;
    }

    GFXfont const & font() const {
        return * font_;
    }

    Canvas & setFg(Pixel color) {
        fg_ = color;
        return *this;
    }

    Canvas & setBg(Pixel color) {
        bg_ = color;
        return *this;
    }

    Canvas & setFont(GFXfont const & font) {
        font_ = & font;
        return *this;
    }

    /** Fills the whole buffer with given color. 
     
        THe whole buffer fill is optimized so that it uses DMA where available, or if not possible to use DMA, fills the buffer consecutively by increasing memcpy calls. 
     */
    void fill(Pixel color) {
        int dma = dma_claim_unused_channel(false);
        if (dma != -1) {
            Pixel fill[] = { color, color}; // copy the color twice so that we use full DMA bandwidth
            dma_channel_config c = dma_channel_get_default_config(dma); // create default channel config, write does not increment, read does increment, 32bits size
            channel_config_set_read_increment(&c, false);
            channel_config_set_write_increment(&c, true);
            size_t size = width_ * height_;
            if (size % 2 == 0) {
                size /= 2;
                channel_config_set_transfer_data_size(& c, DMA_SIZE_32); // transfer 32 bits
            } else {
                channel_config_set_transfer_data_size(& c, DMA_SIZE_16); // transfer 16 bits
            }
            dma_channel_configure(dma, & c, buffer_, fill, size, true); // start
            // wait for the DMA to finish, unclaim and return
            dma_channel_wait_for_finish_blocking(dma);
            dma_channel_unclaim(dma);
        } else {
            buffer_[0] = color;
            int avail = 1;
            int offset = 1;
            int size = width_ * height_;
            while (avail < size) {
                int todo = std::min(size - offset, avail);
                memcpy(buffer_ + offset, buffer_, todo);
                avail += todo;
            }
        }
    }

    /** Fills given rectangle with color. 
     */
    void fill(Rect r, Pixel color) {
        for (int y = r.top, ye = r.bottom; y < ye; ++y)
            for (int x = r.left, xe = r.right; x < xe; ++x)
                buffer_[toOffset(x, y)] = color;
    }

    void rect(Rect r, Pixel color) {
        for (int x = r.left; x < r.right; ++x) {
            buffer_[toOffset(x, r.top)] = color;
            buffer_[toOffset(x, r.bottom - 1)] = color;
        }
        for (int y = r.top + 1;y < r.bottom - 1; ++y) {
            buffer_[toOffset(r.left, y)] = color;
            buffer_[toOffset(r.right - 1, y)] = color;
        }
    }

    void rect(Rect r) { return rect(r, fg_); }

    void write(int x, int y, char const * str, int size = 1) {
        while (*str != 0) 
            x += drawGlyph(x, y, fg_, *(str++), font_, size);
    }

    uint8_t const * buffer() const {
        return reinterpret_cast<uint8_t const *>(buffer_);
    }

    size_t bufferSize() const {
        return width_ * height_ * 2;
    }

private:

    /** Converts the X and Y coordinates to the position in the buffer. 
     
        Internally the buffer is organized identically to the ILI9341 driver it is being used with, i.e. a 320 rows of 240 columns each.

     */
    int toOffset(int x, int y) {
        // for left ribbon (320x240)
        //return (width_ - 1 - x) * height_ + height_ - 1 - y; 
        // for right ribbon (320x240)
        return x * height_ + y;
        // bottom ribbon I think (240x320)
        //return y * width_ + x;
    }

    /** Displays a single GFX font glyph. 
     
        X is the left-most column, y is the baseline. Details on how GFX fonts work can be found here and it the GFX repository:

        https://learn.adafruit.com/creating-custom-symbol-font-for-adafruit-gfx-library/understanding-the-font-specification
        https://github.com/adafruit/Adafruit-GFX-Library/blob/master/Adafruit_GFX.cpp
     */
    int drawGlyph(int x, int y, Pixel color, char c, GFXfont const * font, int size) {
        GFXglyph * glyph = font->glyph + (c - font->first);
        uint8_t const * bitmap = font->bitmap + glyph->bitmapOffset;
        int pixelY = y + glyph->yOffset * size;
        int bi = 0;
        uint8_t bits;
        for (int gy = 0; gy < glyph->height; ++gy, pixelY += size) {
            int pixelX = x + glyph->xOffset * size;
            for (int gx = 0; gx < glyph->width; ++gx, pixelX += size, bits <<= 1) {
                if ((bi++ % 8) == 0)
                    bits = *(bitmap++);
                if (bits & 0x80) {
                    if (size == 1)
                        buffer_[toOffset(pixelX, pixelY)] = color;
                    else
                        fill(Rect::XYWH(pixelX, pixelY, size, size), color);
                }
            }
        }
        return glyph->xAdvance * size;
    }


    uint16_t width_ = 320;
    uint16_t height_ = 240;
    Pixel * buffer_ = nullptr;

    Pixel fg_ = Pixel::White();
    Pixel bg_ = Pixel::Black();
    GFXfont const * font_ = nullptr;

}; // Canvas