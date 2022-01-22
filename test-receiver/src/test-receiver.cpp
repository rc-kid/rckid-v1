#include <Arduino.h>

#include "peripherals/pcd8544.h"
#include "peripherals/nrf24l01.h"

/** Chip Pinout
               -- VDD             GND --
               -- (00) PA4   PA3 (16) -- SCK
               -- (01) PA5   PA2 (15) -- MISO
               -- (02) PA6   PA1 (14) -- MOSI
               -- (03) PA7   PA0 (17) -- UPDI
               -- (04) PB5   PC3 (13) -- NRF_RXTX
               -- (05) PB4   PC2 (12) -- NRF_CS
               -- (06) PB3   PC1 (11) -- NRF_IRQ
               -- (07) PB2   PC0 (10) -- DISPLAY_CE
   DISPLAY_RST -- (08) PB1   PB0 (09) -- DISPLAY_DC
 */

static constexpr gpio::Pin DISPLAY_CE = 10;
static constexpr gpio::Pin DISPLAY_DC = 9;
static constexpr gpio::Pin DISPLAY_RST = 8;

static constexpr gpio::Pin NRF_RXTX = 13;
static constexpr gpio::Pin NRF_CS = 12;
static constexpr gpio::Pin NRF_IRQ = 11;

PCD8544<DISPLAY_RST, DISPLAY_CE, DISPLAY_DC> display_;
NRF24L01 radio_{NRF_CS, NRF_RXTX};

uint8_t expectedMsg = 0;
uint16_t received = 0;
uint16_t valid = 0;

unsigned long time = 0;

void setup() {
    spi::initialize();
    display_.enable();
    display_.clear();
    cpu::delay_ms(100);
    display_.write(0,0,"Hello Again");
    gpio::input(NRF_IRQ);
    radio_.initialize("TEST2", "TEST1", 95, NRF24L01::Speed::m1);
    display_.write(0,0,"      recv ");
    display_.write(0,2,"      valid");
    //cpu::delay_ms(10);
    radio_.standby();
    //cpu::delay_ms(10);
    radio_.startReceiver();
    time = millis();
}

void loop() {
    if (digitalRead(NRF_IRQ) == false) {
        radio_.clearIRQ();
        //while (! radio_.status().rxEmpty()) {
            uint8_t buffer[32];
            radio_.receive(buffer,32);
            ++received;
            if (expectedMsg == buffer[0])
                ++valid;
            expectedMsg = buffer[0] + 1;
        //}
    }
    if (millis() - time > 1000) {
        time = millis();
        display_.gotoXY(0,0);
        display_.write(received, ' ');
        display_.gotoXY(0,2);
        display_.write(valid, ' ');
        if (received == 0) {
            display_.gotoXY(8,3);
            display_.write(radio_.status().raw, ' ');
            display_.gotoXY(8,4);
            display_.write(radio_.fifoStatus(), ' ');
            /*
            radio_.powerDown();
            delay(10);
            radio_.initialize("TEST2", "TEST1", 32, 95);
            radio_.startReceiver();
            */
        }
        received = 0;
        valid = 0;
    }
}




#ifdef HAHA

/*



*/


// PA6 is the DAC

constexpr uint8_t POWER = 0;
constexpr uint8_t VCC = 11; // PC1, AIN7 for ADC1

constexpr uint8_t NRF_RXTX = 13;
constexpr uint8_t NRF_CS = 12;
constexpr uint8_t NRF_IRQ = 11;

constexpr uint8_t JOY_X = 3; // PA7, AIN7 for ADC0
constexpr uint8_t JOY_Y = 4; // PB5, AIN8 for ADC0
constexpr uint8_t BUTTONS = 10; // PC0, AIN6 for ADC1

constexpr uint8_t LED = 7;
constexpr uint8_t VIBRATION = UNUSED;

NeopixelStrip<LED, 4> leds;

/** The display - we don't really need the reset pin. Neither can we afford it. 
 */
//PCD8544<UNUSED, DISPLAY_CS, DISPLAY_DC> display;

SSD1306 display{0x3c};

/** The transciever. 
 */
NRF24L01<NRF_CS, NRF_RXTX, NRF_IRQ> radio;

ADXL345 accelerometer{0x53};

/** The controller's profile for a device. 
 
    Size in EEPROM:
       5 name
       1 icon
       5 deviceName
       1 channel
       4 mapping (0 = channel is set to value, 1 = channel is set to control)
      32 channel settings (values or control ids)
    ----
      48
 */
class Profile {
public:
    /** Name of the profile that will be displayed on screen. 
     */ 
    char name[5];
    /** The icon used to identify the profile. The icon is black and white 24x24 pixels.  
     */
    uint8_t icon; 
    /** The name of the device the profile connects to and the communications channel. 
     */
    char deviceName[5];
    uint8_t channel;




    void saveToEEPROM(uint8_t * address) {

    }

    void loadFromEEPROM(uint8_t * address) {

    }


}; // Profile

/** Controller state and its settings. 
 
    Size in EEPROM
       5 name
       2 joystick X min
       2 joystick X range
       2 joystick Y min
       2 joystick Y range
    ----
      13

    
 */
class Controller {
public:

    char name[6] = { ' ', 'A', 'D', 'A', ' ', '\0'};

    uint8_t accelX;
    uint8_t accelY;
    uint8_t accelZ;
    uint8_t joyX;
    uint8_t joyZ;
    uint8_t btnA;
    uint8_t btnB;
    uint8_t btnC;
    uint8_t btnD;
    uint8_t btnJ;
    uint8_t btnPwr;

    uint16_t joyX_min = 0;
    uint16_t joyX_range = 1023;
    uint16_t joyY_min = 0;
    uint16_t joyY_range = 1023;

    void setJoystickX(uint16_t value) {
        // 0..range to 0..255
        // value * 256 / range
        // value * 64 / (range >> 2)
        value -= joyX_min;
        joyX = value * 64 / (joyX_range >> 2);
    }

    void setJoystickY(uint16_t value) {
        value -= joyX_min;
        joyX = value * 64 / (joyX_range >> 2);
    }

    /** Determines the state of the buttons.
     
        This should be pretty straightforward as we have 6 buttons and 10bit precission result, so the 6 most signifficant digits should directly correspond to the actual buttons. We also round the 4 LSBs for better results stability.
     */
    void setButtons(uint16_t value) {
        uint8_t btns = (value >> 4);
        if (btns != 0 && btns != 63 && (value & 0xf >= 8))
            ++btns;
        // TODO
    }

    void initialize() {
        if (eeprom_read_byte(reinterpret_cast<uint8_t*>(0)) == 0xff)
            saveToEEPROM();
        loadFromEEPROM();
    }

    void loadFromEEPROM() {
        for (int i = 0; i < 5; ++i)
            name[i] = eeprom_read_byte(reinterpret_cast<uint8_t*>(i));
        joyX_min = eeprom_read_word(reinterpret_cast<uint16_t*>(5));
        joyX_range = eeprom_read_word(reinterpret_cast<uint16_t*>(7));
        joyY_min = eeprom_read_word(reinterpret_cast<uint16_t*>(9));
        joyY_range = eeprom_read_word(reinterpret_cast<uint16_t*>(11));
    }

    void saveToEEPROM() {
        for (int i = 0; i < 5; ++i)
            eeprom_write_byte(reinterpret_cast<uint8_t*>(i), name[i]);
        eeprom_write_word(reinterpret_cast<uint16_t*>(5), joyX_min);
        eeprom_write_word(reinterpret_cast<uint16_t*>(7), joyX_range);
        eeprom_write_word(reinterpret_cast<uint16_t*>(9), joyY_min);
        eeprom_write_word(reinterpret_cast<uint16_t*>(11), joyY_range);
    }


}; // Controller

Controller controller;

/** Initializes the ADCs. 
 
    ADC0 alternates between getting the X and Y position of the joystick, while ADC1 alternates between sampling the buttons and the VCC to determine the battery level. 
 */
void initializeADC() {
    // first initialize the input pins:
    static_assert(JOY_X == 3, "JOY_X must be pin 3, PA7, AIN7 for ADC0");
    PORTA.PIN7CTRL &= ~PORT_ISC_gm;
    PORTA.PIN7CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN7CTRL &= ~PORT_PULLUPEN_bm;    
    static_assert(JOY_Y == 4, "JOY_Y must be pin 4, PB5, AIN8 for ADC0");
    PORTB.PIN5CTRL &= ~PORT_ISC_gm;
    PORTB.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN5CTRL &= ~PORT_PULLUPEN_bm;    
    static_assert(BUTTONS == 10, "BUTTONS must be pin 10, PC0, AIN6 for ADC1");
    PORTC.PIN0CTRL &= ~PORT_ISC_gm;
    PORTC.PIN0CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTC.PIN0CTRL &= ~PORT_PULLUPEN_bm;    
    static_assert(VCC == 11, "VCC must be pin 11, PC1, AIN7 for ADC1");
    PORTC.PIN1CTRL &= ~PORT_ISC_gm;
    PORTC.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTC.PIN1CTRL &= ~PORT_PULLUPEN_bm;    
    // enable ADC0
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
    ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
    ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
    ADC0.CTRLD = ADC_INITDLY0_bm;
    ADC0.SAMPCTRL = 0;
    ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;
    // enable ADC1
    ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
    ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc;
    ADC1.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
    ADC1.CTRLD = ADC_INITDLY0_bm;
    ADC1.SAMPCTRL = 0;
    ADC1.MUXPOS = ADC_MUXPOS_AIN6_gc;
    // once both are enabled, start reading the values
    ADC0.COMMAND = ADC_STCONV_bm;
    ADC1.COMMAND = ADC_STCONV_bm;
}

uint8_t x = 0;

/** Checks if the ADCs have finished and processes the results if they did. 
 */
void checkADC() {
    // the joystick axis measurements
    if (ADC0.INTFLAGS & ADC_RESRDY_bm) {
        //high<LED>();
        uint16_t v = ADC0.RES / 64;
        if (ADC0.MUXPOS == ADC_MUXPOS_AIN7_gc) { // JOY_X
            controller.setJoystickX(v);
            ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;
        } else { // JOY_Y
            controller.setJoystickY(v);
            ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;
        }
        ADC0.COMMAND = ADC_STCONV_bm;
        //low<LED>();
        controller.accelX = accelerometer.readX();
        controller.accelY = accelerometer.readY();
        controller.accelZ = accelerometer.readZ();
        /*
        if (++x = 100) {
            x = 0;
            display.write(0,3,"X:");
            display.write(controller.accelX, ' ');
            display.write(0,4,"Y:");
            display.write(controller.accelY, ' ');
            display.write(0,5,"Z:");
            display.write(controller.accelZ, ' ');
        }
        */
    }
    // the buttons and vcc measurements
    if (ADC1.INTFLAGS & ADC_RESRDY_bm) {
        uint16_t v = ADC1.RES / 64;
        if (ADC1.MUXPOS == ADC_MUXPOS_AIN6_gc) { // BUTTONS
            controller.setButtons(v);
            ADC1.MUXPOS = ADC_MUXPOS_AIN7_gc;
        } else { // VCC

            ADC1.MUXPOS = ADC_MUXPOS_AIN6_gc;
        }

        ADC1.COMMAND = ADC_STCONV_bm;
    }
}

void setup() {
    // start the LEDs
    leds.fill(Color::RGB(10,10,10));
    leds.update();

    i2c::initialize();
    display.initialize128x32();
    accelerometer.enable();
    display.clear64();



    /*
    // first initialize SPI
    spi::initialize();
    // initialize the display
    display.enable();
    display.clear();
    // TODO show progress bar

    // initialize the controller
    controller.initialize();

    i2c::initialize();
    accelerometer.enable();


    display.writeLarge(12,1,controller.name);

    // initialize the ADCs and start conversion
    initializeADC();

    // initialize the radio
    radio.initialize(controller.name, 63);
    radio.enableReceiver();
    */
}

void loop() {
    /*
    checkADC();
    display.gotoXY(0,0);
    display.write((uint16_t)x);
    x += 1;
    /*
    if (ADC1.INTFLAGS & ADC_RESRDY_bm) {
        uint16_t x = ADC1.RES / 64;
        display.gotoXY(0,5);
        display.write(x, ' ');
        ADC1.COMMAND = ADC_STCONV_bm;
    }    
    display.gotoXY(0,0);
    display.write('-');
    delay(100);
    display.gotoXY(0,0);
    display.write('|');
    delay(100);
    */
}

#endif
