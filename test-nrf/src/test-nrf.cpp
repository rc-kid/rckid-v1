
#include "platform/platform.h"
#include "peripherals/nrf24l01.h"
#include "peripherals/ssd1306.h"

/** Chip Pinout
               -- VDD             GND --
               -- (00) PA4   PA3 (16) -- SCK
               -- (01) PA5   PA2 (15) -- MISO
               -- (02) PA6   PA1 (14) -- MOSI
               -- (03) PA7   PA0 (17) -- UPDI
               -- (04) PB5   PC3 (13) -- NRF_CS
               -- (05) PB4   PC2 (12) -- NRF_RXTX
               -- (06) PB3   PC1 (11) -- NRF_IRQ
               -- (07) PB2   PC0 (10) -- 
           SDA -- (08) PB1   PB0 (09) -- SCL


    9 free pins

    2 = motor 1
    2 = motor 2
    1 = pwm
    1 = 
 */
constexpr gpio::Pin NRF_CS = 13;
constexpr gpio::Pin NRF_RXTX = 12;
constexpr gpio::Pin NRF_IRQ = 11;

constexpr gpio::Pin DEBUG_PIN = 10;

NRF24L01 radio{NRF_CS, NRF_RXTX};
SSD1306 oled;

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1


volatile bool radio_irq = false;
volatile bool tick = false;
uint8_t ticks = 0;
uint16_t received = 0;
uint16_t errors = 0;
uint8_t x = 0;
bool tickMark = false;

void radioIrq() {
    radio_irq = true;
    gpio::high(DEBUG_PIN);
}

ISR(TCB0_INT_vect) {
    TCB0.INTFLAGS = TCB_CAPT_bm;
    if (++ticks >= 100) {
        tick = true;
        ticks -= 100;
    }
}

void setup() {
    cpu::delay_ms(100);
    gpio::initialize();
    spi::initialize();
    i2c::initializeMaster();
  
    oled.initialize128x32();
    oled.normalMode();
    oled.clear32();

    gpio::output(DEBUG_PIN);
    gpio::low(DEBUG_PIN);

    if (! radio.initialize("TEST2", "TEST1")) {
        oled.write(0,0,"Radio FAIL");
    } else {
        oled.write(0,0,"Radio OK");
    }
    gpio::input(NRF_IRQ);
    attachInterrupt(digitalPinToInterrupt(NRF_IRQ), radioIrq, FALLING);

    radio.standby();
    radio.enableReceiver();

    // start the 1kHz timer for ticks
    // TODO change this to 8kHz for audio recording, or use different timer? 
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CCMP = 50000; // for 100Hz    
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
    oled.write(0,0, "? Recvd  ");
    oled.write(64,0, "Errors");
}

void loop() {
    if (radio_irq) {
        radio_irq = false;
        radio.clearDataReadyIrq();
        uint8_t msg[32];
        while (radio.receive(msg, 32)) {
            received += 1;
            while (++x != msg[0])
                ++errors;
            //x = msg[0];
        }
        gpio::low(DEBUG_PIN);
    }
    if (tick) {
        tickMark = ! tickMark;
        tick = false;
        oled.gotoXY(0,0);
        oled.writeChar(tickMark ? '-' : '|');
        oled.gotoXY(0,1);
        oled.write(received, ' ');
        oled.gotoXY(64,1);
        oled.write(errors, ' ');
        oled.gotoXY(0,2);
        oled.write(radio.getStatus().raw, ' ');
        oled.gotoXY(50,2);
        oled.write(radio.readRegister(NRF24L01::CONFIG), ' ');
        received = 0;
        errors = 0;
    }
}
