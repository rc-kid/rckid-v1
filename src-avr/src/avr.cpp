#include <Arduino.h>

#include "platform/platform.h"
#include "platform/gpio.h"
#include "platform/i2c.h"

#include "comms.h"

/** Chip Pinout
               -- VDD             GND --
               -- (00) PA4   PA3 (16) -- 
               -- (01) PA5   PA2 (15) -- 
               -- (02) PA6   PA1 (14) -- 
               -- (03) PA7   PA0 (17) -- UPDI
               -- (04) PB5   PC3 (13) -- 
               -- (05) PB4   PC2 (12) -- 
               -- (06) PB3   PC1 (11) -- DEBUG
               -- (07) PB2   PC0 (10) -- IRQ
     SDA (I2C) -- (08) PB1   PB0 (09) -- SCL (I2C)

    The AVR is responsible for monitoring the physical inputs and power management. It communicates with RP2040 via I2C and an IRQ pin that AVR rises when there is an input change. 
 */

/* Pins Available: 15

   - 3v3en for pico
   - irq for pico
   - X and Y analog
   - A B C D buttons
   - L R buttons
   - PWR button
   - headphones 
   - battery voltage (?)
   - 2

 */


static constexpr gpio::Pin DEBUG_PIN = 11;

static constexpr gpio::Pin IRQ_PIN = 10;


State state;

/** I2C Handling. 
 */
uint8_t i2cBuffer[32];
uint8_t i2cBytesRecvd = 0;

/** If true, I2C command has been received and should be acted upon in the main loop. 
 */
volatile bool i2cReady = false;
uint8_t i2cBytesSent = 0;


void setup() {
    gpio::output(DEBUG_PIN);
    gpio::low(DEBUG_PIN);

    gpio::input(IRQ_PIN);

    i2c::initializeSlave(AVR_I2C_ADDRESS);

}

void processCommand() {
    // TODO

    i2cBytesRecvd = 0;
    i2cReady = false;
}


void loop() {
    if (i2cReady)
        processCommand();

}


#define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
#define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
#define I2C_DATA_RX (TWI_DIF_bm)
#define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
#define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
#define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
#define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
#define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
#define I2C_STOP_RX (TWI_APIF_bm)

/** Handling of I2C Requests
 */
ISR(TWI0_TWIS_vect) {
    //gpio::high(DEBUG_PIN);
    uint8_t status = TWI0.SSTATUS;
    // sending data to accepting master is on our fastpath as is checked first, if there is more state to send, send next byte, otherwise go to transcaction completed mode. 
    if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
        if (i2cBytesSent < sizeof (State)) {
            TWI0.SDATA = reinterpret_cast<uint8_t*>(& state)[i2cBytesSent++];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        } else {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        }
    // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
    } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
        i2cBuffer[i2cBytesRecvd++] = TWI0.SDATA;
        TWI0.SCTRLB = (i2cBytesRecvd == sizeof(i2cBuffer)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
    // master requests slave to write data, clear the IRQ and prepare to send the state
    } else if ((status & I2C_START_MASK) == I2C_START_TX) {
        gpio::input(IRQ_PIN);
        i2cBytesSent = 0;
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
    // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise
    } else if ((status & I2C_START_MASK) == I2C_START_RX) {
        TWI0.SCTRLB = i2cReady ? TWI_ACKACT_NACK_gc : TWI_SCMD_RESPONSE_gc;
    // sending finished
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
    // receiving finished, inform main loop we have message waiting
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        i2cReady = true;
    } else {
        // error - a state we do not know how to handle
    }
    //gpio::low(DEBUG_PIN);
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
