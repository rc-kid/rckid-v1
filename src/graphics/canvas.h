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

    uint16_t width_ = 320;
    uint16_t height_ = 240;
    Pixel * buffer_ = nullptr;
}; // Canvas