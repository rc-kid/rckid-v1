#pragma once

#include <stdint.h>

class Pixel {
public:
    Pixel() = default;

private:
    uint16_t pixel_ = 0;
}; // Pixel

/** The display driver and framebuffer. 
 
    The controlled display is expected to be a 176x220 TFT SPI display with ILI9225 controller and SPI driver, such as this one:

    https://www.laskakit.cz/176x220-barevny-lcd-tft-displej-2-0--spi/

 */
class Display {

}; // Display