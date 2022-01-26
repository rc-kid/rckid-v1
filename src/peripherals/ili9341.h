#pragma once
#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include "platform/gpio.h"

class Pixel {
private:
    uint16_t data_;
}; // Pixel


/** The ILI9341 SPI display driver. 

    We need a decent framerate for the entire display. That is 320x240x16x24 (full screen, 16bit color, 24 fps), i.e. 28.125Mbps. The controller only has about ~10Mbps SPI bandwidth, which is not enough. The internet says that this *can* be pushed up to 40Mbps, so technically doable, but the buffer would be locked for almost all time. 

    The parallel interface can be used as an alternative. A minimal write cycle is 66ns, on RP2040 this translates to 12 cycles for ~90ns per byte, around 14ms, out of ~40ms available for the frame, so pretty doable. 

    Since we want the optimal DMA bandwidth, give the data & WRT pins to pio only when in use, disable between block transfers.

    40pin connector:

    1,2,3,4 = Touch panel
    5 = GND
    6 = VCC
    & = ??
    8 = FMARK (tearing effect)
    9 = CS
    10 = RS/SP (1 == data, 0 = command for parallel), SCK for SPI
    11 = WR/A0 (write at rising edge for parallel), data or command for SPI, VCC if not used
    12 = RD (read at rising edge), VCC if not used
    13 = SPI SDI/SDA, if not used, fix at VCC or GND
    14 = SPI SD0, data outputted on falling edge of SCL, leave floating if not used
    15 = RESET, active low
    16 = GND
    17-24 = Data bus DBO - DB7, GND if not used
    25-32 = Data bus DB8-DB15, GND if not used
    33 = LED Anode
    34,35,36 = LED Cathode (connected together)
    37 = GND
    38, 39, 40 = IM0, IM1, IM2

    For the purposes of driving the display from RP2040 in parallel interface, the following should be connected:

    5 = GND
    6 = VCC
    8 = FMARK 
    9 = CS
    10 = RS/SP
    11 = WR
    12 = 6 - VCC (RD is not used)  
    13 = 6 - VCC (SPI SDI/SDA not used)
    15 = reset ???
    16 = GND
    17-24 = GND (first data bank, not used)
    25-32 = Data
    33 = 6 - VCC (LED anode)
    34, 35, 36 = R47 to GND ? (via transistor for PWM baclight intensity)
    37 = GND
    38 = 6 (VCC for 8bit parallel interface II)
    39 = 37 (GND, for 8bit parallel interface)
    40 = 37 (GND, for 8bit parallel interface)

    Actual connections are: 5,6,8,9,10,11,25-32
 */
template<gpio::Pin CS, gpio::Pin DC, gpio::Pin FMARK, gpio::Pin WR, gpio::Pin DATA>
class ILI9341 {
public:

    /** Creates the display driver and initializes the pins. 
     
        All pins are set to ouput, data pins and WRX are set to PIO, other pins are controlled by the main cpu. Loads the PIO code. 
     */
    ILI9341() {
    }

    void initialize() {
        // initialize pins MCU driven pins
        gpio_init(CS);
        gpio_set_dir(CS, GPIO_OUT);
        gpio_put(CS, true);
        gpio_init(DC);
        gpio_set_dir(DC, GPIO_OUT);
        gpio_put(DC, true);
        gpio_init(FMARK);
        gpio_set_dir(FMARK, GPIO_IN);
        // TODO debugging purposes only, initialize WR and data pins to MCU
        //gpio_init_mask(0x00000000000000ff << DATA);
        gpio_init(WR);
        gpio_set_dir(WR, GPIO_OUT);
        gpio_put(WR, true);

        gpio_init_mask(0xff00);
        gpio_set_dir_out_masked(0xff00);
        // initialize PIO driven pins (data and WRX pin)
        /*
        pio_gpio_init(pio0, WRX_PIN);
        for (unsigned i = 0; i < 8; ++i)
            pio_gpio_init(pio0, DATA_START_PIN + i);
        */
        sendCommandSequence(initcmd);

        /*
        sendCommand(SLEEP_OUT, nullptr, 0);
        sleep_ms(150);
        sendCommand(DISPLAY_ON, nullptr, 0);
        sleep_ms(150);
        */
        for (int i = 0; i < 5; ++i) {
            sendCommand(INVERSE_ON, nullptr, 0);
            //sendCommand(0b10000100, nullptr, 0);
            sleep_ms(500);
            sendCommand(INVERSE_OFF, nullptr, 0);
            //sendCommand(0b00000100, nullptr, 0);
            sleep_ms(500);
        }
    }


    /** Updates the entire display, i.e. 320x240 pixels. 
     */
    /*
    void update(Pixel * data) {
        update(data, 0, 0, 319, 239);
    }
    */

    /** Updates the selected part of the display. 
     */
    /*
    void update(Pixel * data, unsigned left, unsigned top, unsigned right, unsigned bottom);
    */


private:

#define ILI9341_TFTWIDTH 240  ///< ILI9341 max TFT width
#define ILI9341_TFTHEIGHT 320 ///< ILI9341 max TFT height

#define ILI9341_NOP 0x00     ///< No-op register
#define ILI9341_SWRESET 0x01 ///< Software reset register
#define ILI9341_RDDID 0x04   ///< Read display identification information
#define ILI9341_RDDST 0x09   ///< Read Display Status

#define ILI9341_SLPIN 0x10  ///< Enter Sleep Mode
#define ILI9341_SLPOUT 0x11 ///< Sleep Out
#define ILI9341_PTLON 0x12  ///< Partial Mode ON
#define ILI9341_NORON 0x13  ///< Normal Display Mode ON

#define ILI9341_RDMODE 0x0A     ///< Read Display Power Mode
#define ILI9341_RDMADCTL 0x0B   ///< Read Display MADCTL
#define ILI9341_RDPIXFMT 0x0C   ///< Read Display Pixel Format
#define ILI9341_RDIMGFMT 0x0D   ///< Read Display Image Format
#define ILI9341_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result

#define ILI9341_INVOFF 0x20   ///< Display Inversion OFF
#define ILI9341_INVON 0x21    ///< Display Inversion ON
#define ILI9341_GAMMASET 0x26 ///< Gamma Set
#define ILI9341_DISPOFF 0x28  ///< Display OFF
#define ILI9341_DISPON 0x29   ///< Display ON

#define ILI9341_CASET 0x2A ///< Column Address Set
#define ILI9341_PASET 0x2B ///< Page Address Set
#define ILI9341_RAMWR 0x2C ///< Memory Write
#define ILI9341_RAMRD 0x2E ///< Memory Read

#define ILI9341_PTLAR 0x30    ///< Partial Area
#define ILI9341_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define ILI9341_MADCTL 0x36   ///< Memory Access Control
#define ILI9341_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define ILI9341_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set

#define ILI9341_FRMCTR1                                                        \
  0xB1 ///< Frame Rate Control (In Normal Mode/Full Colors)
#define ILI9341_FRMCTR2 0xB2 ///< Frame Rate Control (In Idle Mode/8 colors)
#define ILI9341_FRMCTR3                                                        \
  0xB3 ///< Frame Rate control (In Partial Mode/Full Colors)
#define ILI9341_INVCTR 0xB4  ///< Display Inversion Control
#define ILI9341_DFUNCTR 0xB6 ///< Display Function Control

#define ILI9341_PWCTR1 0xC0 ///< Power Control 1
#define ILI9341_PWCTR2 0xC1 ///< Power Control 2
#define ILI9341_PWCTR3 0xC2 ///< Power Control 3
#define ILI9341_PWCTR4 0xC3 ///< Power Control 4
#define ILI9341_PWCTR5 0xC4 ///< Power Control 5
#define ILI9341_VMCTR1 0xC5 ///< VCOM Control 1
#define ILI9341_VMCTR2 0xC7 ///< VCOM Control 2

#define ILI9341_RDID1 0xDA ///< Read ID 1
#define ILI9341_RDID2 0xDB ///< Read ID 2
#define ILI9341_RDID3 0xDC ///< Read ID 3
#define ILI9341_RDID4 0xDD ///< Read ID 4

#define ILI9341_GMCTRP1 0xE0 ///< Positive Gamma Correction
#define ILI9341_GMCTRN1 0xE1 ///< Negative Gamma Correction

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
  ILI9341_SLPOUT  , 0x80,                // Exit Sleep
  ILI9341_DISPON  , 0x80,                // Display on
  0x00                                   // End of list
  };

    uint8_t const * sendCommand(uint8_t const * cmdSequence) {
        uint8_t cmd = cmdSequence[0];
        uint8_t nargs = cmdSequence[1] & 0x7f;
        sendCommand(cmd, cmdSequence + 2, nargs);
        if (cmdSequence[1] >= 0x80)
            sleep_ms(150);
        return cmdSequence + 2 + nargs;
    }

    void sendCommandSequence(uint8_t const * sequence) {
        while (sequence[0] != 0)
            sequence = sendCommand(sequence);
    }

    static constexpr uint8_t RESET = 0x01;
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
    static constexpr uint8_t MADCTL = 0x36;
    static constexpr uint8_t VERTICAL_SCROLL_START = 0x37;
    static constexpr uint8_t PIXEL_FORMAT = 0x3a;

    void sendCommand(uint8_t command, uint8_t const * data, unsigned length) {
        gpio_put(CS, false);
        gpio_put(DC, false);
        printf("Command %02x", command);
        gpio_put_masked(0x00000000000000ff << DATA, static_cast<uint64_t>(command) << DATA);
        gpio_put(WR, false);
        sleep_us(1);
        gpio_put(WR, true);
        sleep_us(10);
        gpio_put(DC, true);
        while (length-- > 0) {
            printf(" %02x", *data);
            gpio_put_masked(0xff << DATA, *(data++) << DATA);
            gpio_put(WR, false);
            sleep_us(1);// wait
            gpio_put(WR, true);
            sleep_us(10);
        }
        printf("\n");
        gpio_put(CS, true);
    }

}; // ILI9341