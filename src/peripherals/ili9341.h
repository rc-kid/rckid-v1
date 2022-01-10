#pragma once
#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/pio.h>

class Pixel {
private:
    uint16_t data_;
}; // Pixel


/** The ILI9341 SPI display driver. 

    We need a decent framerate for the entire display. That is 320x240x16x24 (full screen, 16bit color, 24 fps), i.e. 28.125Mbps. The controller only has about ~10Mbps SPI bandwidth, which is not enough. The internet says that this *can* be pushed up to 40Mbps, so technically doable, but the buffer would be locked for almost all time. 

    The parallel interface can be used as an alternative. A minimal write cycle is 66ns, on RP2040 this translates to 12 cycles for ~90ns per byte, around 14ms, out of ~40ms available for the frame, so pretty doable. 

    Since we want the optimal DMA bandwidth, give the data & WRT pins to pio only when in use, disable between block transfers.

 */
class ILI9341 {
public:
    static constexpr unsigned CSX_PIN = 5;
    static constexpr unsigned DCX_PIN = 6;
    static constexpr unsigned WRX_PIN = 7;
    static constexpr unsigned DATA_START_PIN = 8;
    static constexpr unsigned PIO_SM = 0;

    /** Initializes the display. 
     
        Assigns the pins, loads the pio driver and initializes the display. 
     */
    void initialize();

    /** Updates the entire display, i.e. 320x240 pixels. 
     */
    void update(Pixel * data) {
        update(data, 0, 0, 319, 239);
    }

    /** Updates the selected part of the display. 
     */
    void update(Pixel * data, unsigned left, unsigned top, unsigned right, unsigned bottom);



private:

    static constexpr uint8_t DISPLAY_OFF = 0x28;
    static constexpr uint8_t DISPLAY_ON = 0x29;
    static constexpr uint8_t COLUMN_ADDRESS_SET = 0x2a;
    static constexpr uint8_t PAGE_ADDRESS_SET = 0x2b;

    void dataToMCU() {
        gpio_init(WRX_PIN);
        gpio_set_dir(WRX_PIN, GPIO_OUT);
        gpio_put(WRX_PIN, true);
        gpio_init_mask(0xff << DATA_START_PIN);
        gpio_set_dir_masked(0xff << DATA_START_PIN, GPIO_OUT);
    }

    void dataToPIO() {
        pio_gpio_init(pio0, WRX_PIN);
        for (unsigned i = 0; i < 8; ++i)
            pio_gpio_init(pio0, DATA_START_PIN + i);
        // TODO
    }

    void initializePin(unsigned pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, true);
    }

    void sendCommand(uint8_t command, uint8_t * data, unsigned length) {
        gpio_put(CSX_PIN, false);
        gpio_put(DCX_PIN, false);
        gpio_put_masked(0xff << DATA_START_PIN, command << DATA_START_PIN);
        gpio_put(WRX_PIN, false);
        // wait
        gpio_put(WRX_PIN, true);
        gpio_put(DCX_PIN, true);
        while (length-- > 0) {
            gpio_put_masked(0xff << DATA_START_PIN, *(data++) << DATA_START_PIN);
            gpio_put(WRX_PIN, false);
            // wait
            gpio_put(WRX_PIN, true);
        }
        gpio_put(CSX_PIN, true);
    }

}; // ILI9341