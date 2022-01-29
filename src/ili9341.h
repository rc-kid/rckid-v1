#pragma once

#include "platform/gpio.h"
#include "platform/spi.h"

#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include "ili9341.pio.h"
#include "graphics.h"


enum class DisplayRotation {
    Left,
    Top,
    Right,
    Bottom,
}; 

/** ILI9341
 
    The driver provides the higher-levekl API of the display and is templated by the interface providers described below that are responsible for the actual implementation of the low level protocol. 
 
    # Wiring

    I am using the 40pin 0.5mm displays, such as (...). The interface looks like this:

    Pin(s)  | Unused | Description
    ------- | ------ | ----------------  
    1-4     |        | UNUSED (touchscreen)
    5       | GND    | GND 
    6       | VCC    | VCC
    7       | VCC    | VCC? (the datasheet sets RESET, but then IOVCC is twice, pulling high should be safe)
    8       | float  | FMARK (floating if not used)
    9       |        | CS for both SPI and 8080
    10      |        | SCLK for SPI, DC for 8080 (data = 1, command = 0)
    11      | VCC    | WR for 8080, D/CX for 4 line SPI
    12      | VCC    | RD for 8080
    13      | VCC    | SPI MOSI
    14      | float  | SPI MISO
    15      | VCC?   | Reset, active low
    16      | GND    | GND
    17-24   | GND    | DB0-DB7 for 8080
    25-32   | GND    | DB8-DB15 for 8080
    33      | VCC    | Backlight anode (VCC)
    34-36   |        | BAcklight cathode (GND via a resistor, all the same wire)  
    37      | GND    | GND
    38      |        | IM0
    39      |        | IM1
    40      |        | IM2

    # Communication modes
    
    I believe that IM3 is set to 1 internally, the following modes are of interest:

    IM2 | IM1 | IM0 | Mode
    --- | --- | --- | ---------------
     0  |  0  |  1  | 8bit parallel II (DB15-DB8 pins should be used)
     1  |  1  |  0  | 4 wire SPI (8bit with D/C)

    # Wiring up

    First check the LED, it should turn the display all white. 

 */
template<typename T, gpio::Pin FMARK = gpio::UNUSED>
class ILI9341 : public T {
public:

    /** For debugging purposes only, prints the display information. 
     */
    void debug() {
        beginCommand(READ_DISPLAY_IDENTIFICATION);
        data();
        read();
        uint8_t id0 = read();
        uint8_t id1 = read();
        uint8_t id2 = read();
        end();
        printf("CS should be up\n");
        printf("ID1: %02x\n", id0);
        printf("ID2: %02x\n", id1);
        printf("ID3: %02x\n", id2);
    }

    /** Initializes the display interface.

        Note that this does not turn the display on yet, for that call the initializeDisplay() method instead. This method simply sets the pins and loads any required structures (such as PIO code for RP2040).   
     */
    void initialize() {
        T::initialize();
        if (FMARK != gpio::UNUSED)
            gpio::input(FMARK);
    }

    /** Initializes the display. 
     */
    void initializeDisplay(DisplayRotation r) {
        debug();
        sendCommand8(VERTICAL_SCROLL_START, 0);
        sendCommand8(PIXEL_FORMAT, PIXEL_FORMAT_16);
        rotate(r);
        sendCommand8(TEARING_LINE_ON, TEARING_LINE_MODE_VSYNC);
        sendCommand(SLEEP_OUT, nullptr, 0);
        cpu::delay_ms(150);
        sendCommand(DISPLAY_ON, nullptr, 0);
        cpu::delay_ms(150);
        //sendCommandSequence(initcmd);

        // fill the display black
        //fill(Pixel{0,0,31});
        /*
        for (int i = 0; i < 5;++i) {
            sendCommand(INVERSE_ON, nullptr, 0);
            cpu::delay_ms(500);
            sendCommand(INVERSE_OFF, nullptr, 0);
            cpu::delay_ms(500);
        }
        */

    }

    // right is top
    // left is bottom
    // bottom is left
    // top is right


    void rotate(DisplayRotation r) {
        uint8_t madctl;
        switch (r) {
            case DisplayRotation::Left:
                madctl = MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR;
                break;
            case DisplayRotation::Top:
                madctl = MADCTL_MY | MADCTL_BGR;
                break;
            case DisplayRotation::Right:
                madctl = MADCTL_MV | MADCTL_BGR;
                break;
            case DisplayRotation::Bottom:
                madctl = MADCTL_MX | MADCTL_BGR;
                break;
        }
        sendCommand8(MADCTL, madctl);
    }

    /** Fills given area of the display with provided pixel data. 
     
        Accepts size of the buffer, which must be a power of two. If the size is smaller than the actual area to be written, provided pixel data will be wrapped. 
     */
    void fill(Rect r, uint8_t const * pixelData, size_t pixelDataSize = 0) {
        /*
        if (FMARK != gpio::UNUSED) {
            while (!gpio_get(FMARK));
            //while (gpio_get(FMARK));
        }
        */
        setColumnUpdateRange(r.left, r.right - 1);
        setRowUpdateRange(r.top, r.bottom - 1);
        beginCommand(RAM_WRITE);
        data();
        if (FMARK != gpio::UNUSED) {
            while (!gpio_get(FMARK));
            //while (gpio_get(FMARK));
        }
        if (writeAsync(pixelData, r.width() * r.height() * 2, pixelDataSize))
            writeAsyncWait();
        end();    
    }

    /** Fills the entire screet with given color. 
     */
    void fill(Pixel p) {
        setColumnUpdateRange();
        setRowUpdateRange();
        beginCommand(RAM_WRITE);
        data();
        p = Pixel{0};
        for (int i = 0; i < 240; ++i) {
            for (int j = 0; j < 320; ++j) {
                if (j <= 240 - i) 
                    p.r(i % 32).b(j % 32).g(0);
                else
                    p.r(0).b(0).gFull(j % 64);

                write(static_cast<uint16_t>(p) >> 8);
                write(static_cast<uint16_t>(p) & 0xff);
            }
        }
        end();
    }

    void setColumnUpdateRange(unsigned start = 0, unsigned end = 319) {
        sendCommand16(COLUMN_ADDRESS_SET, start, end);
    }

    void setRowUpdateRange(unsigned start = 0, unsigned end = 239) {
        sendCommand16(PAGE_ADDRESS_SET, start, end);
    }

protected:
    using T::initialize;
    using T::begin;
    using T::end;
    using T::data;
    using T::command;
    using T::write;
    using T::writeAsync;
    using T::writeAsyncWait;
    using T::read;

    static constexpr uint8_t RESET = 0x01;
    static constexpr uint8_t READ_DISPLAY_IDENTIFICATION = 0x04;
    static constexpr uint8_t SLEEP_IN = 0x10;
    static constexpr uint8_t SLEEP_OUT = 0x11;
    static constexpr uint8_t PATRTIAL_MODE = 0x12;
    static constexpr uint8_t NORMAL_MODE = 0x13;
    static constexpr uint8_t INVERSE_ON = 0x20;
    static constexpr uint8_t INVERSE_OFF = 0x21;
    static constexpr uint8_t GAMMA_SET = 0x26;
    static constexpr uint8_t DISPLAY_OFF = 0x28;
    static constexpr uint8_t DISPLAY_ON = 0x29;
    static constexpr uint8_t COLUMN_ADDRESS_SET = 0x2a;
    static constexpr uint8_t PAGE_ADDRESS_SET = 0x2b;
    static constexpr uint8_t RAM_WRITE = 0x2c;
    static constexpr uint8_t PARTIAL_AREA = 0x30;
    static constexpr uint8_t VERTICAL_SCROLL = 0x33;
    static constexpr uint8_t TEARING_LINE_OFF = 0x34;
    static constexpr uint8_t TEARING_LINE_ON = 0x35;
    static constexpr uint8_t TEARING_LINE_MODE_VSYNC = 0;
    static constexpr uint8_t TEARING_LINE_MODE_V_AND_H_SYNC = 1;
    static constexpr uint8_t MADCTL = 0x36;
    static constexpr uint8_t MADCTL_MY = 0x80;  // Bottom to top
    static constexpr uint8_t MADCTL_MX = 0x40;  // Right to left
    static constexpr uint8_t MADCTL_MV = 0x20;  // Reverse Mode
    static constexpr uint8_t MADCTL_ML = 0x10;  // LCD refresh Bottom to top
    static constexpr uint8_t MADCTL_RGB = 0x00; // Red-Green-Blue pixel order
    static constexpr uint8_t MADCTL_BGR = 0x08; // Blue-Green-Red pixel order
    static constexpr uint8_t MADCTL_MH = 0x04;  // LCD refresh right to left    
    static constexpr uint8_t VERTICAL_SCROLL_START = 0x37;
    static constexpr uint8_t PIXEL_FORMAT = 0x3a;
    static constexpr uint8_t PIXEL_FORMAT_16 = 0x55; // 16 bit pixel format
    static constexpr uint8_t SET_TEAR_SCANLINE = 0x44;

    void beginCommand(uint8_t cmd) {
        begin();
        command();
        write(cmd);
    }

    void sendCommand(uint8_t cmd, uint8_t const * params, uint8_t size) {
        beginCommand(cmd);
        if (size > 0) {
            data();
            while (size-- != 0) 
                write(*(params++));
        }
        end();
    }

    void sendCommand8(uint8_t cmd, uint8_t p) {
        sendCommand(cmd, & p, 1);
    }
    void sendCommand16(uint8_t cmd, uint16_t p1) {
        uint16_t params = swapBytes(p1);
        sendCommand(cmd, reinterpret_cast<uint8_t *>( & params), 2);
    }

    void sendCommand16(uint8_t cmd, uint16_t p1, uint16_t p2) {
        uint16_t params[] = { swapBytes(p1), swapBytes(p2) };
        sendCommand(cmd, reinterpret_cast<uint8_t *>(params), 4);
    }

}; // ILI9341

/** ILI9341 Display driver using the SPI interface. 
 
    A very simple fallback driver that uses the 4-wire SPI interface. Uses the platform interface so that the code itself is portable. 

    # Wiring

    Pin(s)  | Connection | Description
    ------- | ---------- | ----------------  
    5       | GND        | GND 
    6       | VCC        | VCC
    7       | VCC        | VCC
    8       | float      | FMARK
    9       | gpio       | CS
    10      | gpio       | SPI CLK
    11      | gpio       | DC
    12      | VCC        | RD for 8080
    13      | gpio       | SPI MOSI
    14      | gpio       | SPI MISO
    15      | VCC        | Reset (unused, active low)
    16      | GND        | GND
    17-32   | GND        | Data for 8080, unused
    33      | VCC        | Backlight anode
    34-36   | GND-RES    | Backlight cathode (GND via a resistor, all the same wire)  
    37      | GND        | GND
    38      | GND        | IM0 to select SPI mode
    39      | VCC        | IM1 to select SPI mode
    40      | VCC        | IM2 to select SPI mode

    # Timings

    > From page 231 of the datasheet, these are the absolute minimums. At 133Mhz, one instruction is 7.5ns

    Name  | [ns/cycles] | Description
    ----- | ----------- | ---------------------------------------
    TCSS  | 40/6        | From CS to start transmission (really to CLK rising)
    TWC   | 100         | CLK cycle (write), i.e. 10 Mhz
    TRC   | 150         | CLK cycle (read), i.e. 6.6 Mhz
    TAS   | 10/2        | DCX setup time (from change to transmission start)
    TAH   | 10/2        | DCX hold time (from transmission end to change)

    However, tested that the actual SPI speed can be much much higher, even 80Mhz seemed to work. That said, at this speed the actual rpi code is the bottleneck. 

 */
template<gpio::Pin CS, gpio::Pin DC>
class ILI9341_SPI {
protected:
    ILI9341_SPI() = default;

    void initialize() {
        gpio::output(CS);
        gpio::high(CS);
        gpio::output(DC);
        gpio::high(DC);
    }

    /** Starts communication with the chip by pulling the CS line low. 
     */
    void begin() {
        spi::begin(CS);
    }

    /** Ends communication with the chip by pulling the CS line high. 
     */
    void end() {
        spi::end(CS);
    }

    void data() {
        gpio::high(DC);
    }

    void command() {
        gpio::low(DC);
    }

    void write(uint8_t data) {
        spi::transfer(data);
    }

    /** Async (where supported) write of the given buffer. 

        Always transmits the required bytes. If the transfer is asynchronous, returns true, false is returned when asynrhonous transfer is not supported (and therefore the transfer has already happened).     
     */
    bool writeAsync(uint8_t const * data, size_t size, size_t actualSize = 0) {
#if (defined ARCH_RP2040)
        // on RP2040 we can use DMA for the asynchronous transfer and a higher transfer speed overall
        dma_ = dma_claim_unused_channel(true);
        dma_channel_config c = dma_channel_get_default_config(dma_); // create default channel config, write does not increment, read does increment, 32bits size
        channel_config_set_transfer_data_size(& c, DMA_SIZE_8); // transfer 8 bytes
        channel_config_set_dreq(& c, spi_get_dreq(spi0, true)); // tell SPI
        // determine the wrapping
        if (actualSize != 0) {
            if (actualSize == 1)
                channel_config_set_read_increment(& c, false);
            else
                channel_config_set_ring(& c, false, 31 - __builtin_clz(actualSize)); 
        }        
        dma_channel_configure(dma_, & c, & spi_get_hw(spi0)->dr, data, size, true); // start
        return true;
#else
        if (actualSize == 0) 
            actualSize = size;
        size_t i = 0;
        while (size-- != 0) {
            write(data[i++]);
            if (i == actualSize)
                i = 0;
        }
        return false;
#endif
    }

    /** Returns true if there is an asynchronous data write in process. 
     */
    bool writeAsyncBusy() {
#if (defined ARCH_RP2040)
        return dma_channel_is_busy(dma_);
#endif
        return false;
    }

    /** Waits for the asynchronous data transfer to finish, then returns. 
     
        If there is no such transfer, returns immediately. 
     */
    void writeAsyncWait() {
#if (defined ARCH_RP2040)
        dma_channel_wait_for_finish_blocking(dma_);
        dma_channel_unclaim(dma_);
        dma_ = -1;
        cpu::delay_ms(1);
#endif
        // nop
    }

    /** Reads a single byte from the controller. 
     */
    uint8_t read() {
        return spi::transfer(0);
    }

private:
#if (defined ARCH_RP2040)
    int dma_ = -1;
#endif
}; // ILI9341_SPI


#if (defined ARCH_RP2040)
/** 8bit parallel driver interface for the ILI9341 display controller. 
 
    This driver is specific to RP2040 and won't work on other chips. 

    # Wiring 

    Pin(s)  | Connection | Description
    ------- | ---------- | ----------------  
    5       | GND        | GND 
    6       | VCC        | VCC
    7       | VCC        | VCC
    8       | gpio 6     | FMARK
    9       | gpio 4     | CS
    10      | gpio 5     | DC
    11      | gpio 7     | WR
    12      | VCC        | RD for 8080, unused
    13      | VCC        | SPI MOSI, unused
    14      | float      | SPI MISO, unused
    15      | VCC        | Reset (unused, active low)
    16      | GND        | GND
    17-24   | GND        | DB0-DB7 for 8080, unused
    25-32   | gpio 8-15  | DB8-DB15 for 8080 (gpio 8-15)
    33      | VCC        | Backlight anode
    34-36   | GND-RES    | Backlight cathode (GND via a resistor, all the same wire)  
    37      | GND        | GND
    38      | VCC        | IM0, to select 8bit 8080
    39      | GND        | IM1, to select 8bit 8080
    40      | GND        | IM2, to select 8bit 8080

    # Timings

    > From page 228 of the datasheet.  At 133Mhz, one instruction is 7.5ns

    Name  | [ns/cycles] | Description
    ----- | ----------- | ----------------------
    TWC   | 66/10       | Write cycle
    TWRH  | 15          | WR high duration 
    TWRL  | 15          | WR low duration

 */
template<gpio::Pin CS, gpio::Pin DC, gpio::Pin WR, gpio::Pin FMARK, gpio::Pin DATA, uint PIO_ID = 0> 
class ILI9341_8080 {
protected:
    ILI9341_8080() = default;

    void initialize() {
        gpio_init(CS);
        gpio_set_dir(CS, GPIO_OUT);
        gpio_put(CS, true);
        gpio_init(DC);
        gpio_set_dir(DC, GPIO_OUT);
        gpio_put(DC, true);
        gpio_init(FMARK);
        gpio_set_dir(FMARK, GPIO_IN);
        // load the pio code for writing data, which also initializes the WR and DATA pins
        pio_ = PIO_ID ? pio1 : pio0;
        offset_ = pio_add_program(pio_, &ili9341_program);
        sm_ = pio_claim_unused_sm(pio_, true);
        ili9341_program_init(pio_, sm_, offset_, WR, DATA);
    }

    void begin() {
        ili9341_wait_idle(pio_, sm_);
        gpio_put(CS, false);
    }

    void end() {
        ili9341_wait_idle(pio_, sm_);
        gpio_put(CS, true);
    }

    void data() {
        ili9341_wait_idle(pio_, sm_);
        gpio_put(DC, true);
    }

    void command() {
        ili9341_wait_idle(pio_, sm_);
        gpio_put(DC, false);
    }

    void write(uint8_t data) {
        ili9341_put(pio_, sm_, data);
        /*
        gpio_put(WR, false);
        gpio_put_masked(0xff << DATA, static_cast<uint64_t>(data) << DATA);
        asm volatile("nop \n nop \n nop");
        gpio_put(WR, true);
        asm volatile("nop \n nop \n nop \n nop");
        */
    }

    /** Async (where supported) write of the given buffer. 

        Always transmits the required bytes. If the transfer is asynchronous, returns true, false is returned when asynrhonous transfer is not supported (and therefore the transfer has already happened).     
     */
    bool writeAsync(uint8_t const * data, size_t size, size_t actualSize = 0) {
        dma_ = dma_claim_unused_channel(true);
        dma_channel_config c = dma_channel_get_default_config(dma_); // create default channel config, write does not increment, read does increment, 32bits size
        channel_config_set_transfer_data_size(& c, DMA_SIZE_8); // transfer 8 bytes
        channel_config_set_dreq(& c, pio_get_dreq(pio_, sm_, true)); // tell our PIO
        // determine the wrapping
        if (actualSize != 0) {
            if (actualSize == 1)
                channel_config_set_read_increment(& c, false);
            else
                channel_config_set_ring(& c, false, 31 - __builtin_clz(actualSize)); 
        }        
        dma_channel_configure(dma_, & c, &pio_->txf[sm_], data, size, true); // start
        return true;
    }

    /** Returns true if there is an asynchronous data write in process. 
     
        This means that either the DMA channel is still transferring, or if DMA is done we must make sure that the PIO has stalled on an empty queue. 
     */
    bool writeAsyncBusy() {
        return dma_channel_is_busy(dma_) || ili9341_busy(pio_, sm_);
    }

    /** Waits for the asynchronous data transfer to finish, then returns. 
     
        If there is no such transfer, returns immediately. 
     */
    void writeAsyncWait() {
        dma_channel_wait_for_finish_blocking(dma_);
        dma_channel_unclaim(dma_);
        dma_ = -1;
        ili9341_wait_idle(pio_, sm_);
    }

    // We don't do read in parallel. 
    uint8_t read() { return 0; }

private:
    PIO pio_;
    uint sm_;
    uint offset_;
    int dma_ = -1;

}; // ILI9431_8080
#endif // ARCH_RP2040

/*
static constexpr uint8_t initcmd[] = {
  0xEF, 3, 0x03, 0x80, 0x02,
  0xCF, 3, 0x00, 0xC1, 0x30,
  0xED, 4, 0x64, 0x03, 0x12, 0x81,
  0xE8, 3, 0x85, 0x00, 0x78,
  0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  0xF7, 1, 0x20,
  0xEA, 2, 0x00, 0x00,
  ILI9341_PWCTR1  , 1, 0x23,             // Power control VRH[5:0]
  ILI9341_PWCTR2  , 1, 0x10,             // Power control SAP[2:0];BT[3:0]
  ILI9341_VMCTR1  , 2, 0x3e, 0x28,       // VCM control
  ILI9341_VMCTR2  , 1, 0x86,             // VCM control2
  ILI9341_MADCTL  , 1, 0x48,             // Memory Access Control
  ILI9341_VSCRSADD, 1, 0x00,             // Vertical scroll zero
  ILI9341_PIXFMT  , 1, 0x55,
  ILI9341_FRMCTR1 , 2, 0x00, 0x18,
  ILI9341_DFUNCTR , 3, 0x08, 0x82, 0x27, // Display Function Control
  0xF2, 1, 0x00,                         // 3Gamma Function Disable
  ILI9341_GAMMASET , 1, 0x01,             // Gamma curve selected
  ILI9341_GMCTRP1 , 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
  ILI9341_GMCTRN1 , 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  ILI9341_MADCTL  , 1, 0x48,             // Memory Access Control
  ILI9341_VSCRSADD, 1, 0x00,             // Vertical scroll zero
  ILI9341_PIXFMT  , 1, 0x55,
  ILI9341_SLPOUT  , 0x80,                // Exit Sleep
  ILI9341_DISPON  , 0x80,                // Display on
  0x00                                   // End of list
  };
*/
