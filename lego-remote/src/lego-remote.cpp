#include "platform/platform.h"
#include "utils/debug_display.h"
#include "peripherals/nrf24l01.h"
#include "peripherals/neopixel.h"

/** Chip Pinout
               -- VDD             GND --
     ML1 (WOA) -- (00) PA4   PA3 (16) -- SCK
     MR1 (WOB) -- (01) PA5   PA2 (15) -- MISO
           XL1 -- (02) PA6   PA1 (14) -- MOSI
           XL2 -- (03) PA7   PA0 (17) -- UPDI
           XR1 -- (04) PB5   PC3 (13) -- NRF_CS
           XR2 -- (05) PB4   PC2 (12) -- NRF_RXTX
       NRF_IRQ -- (06) PB3   PC1 (11) -- MR2(WOD)
  NEOPIXEL_PIN -- (07) PB2   PC0 (10) -- ML2(WOC)
           SDA -- (08) PB1   PB0 (09) -- SCL

    Timers

    - 20ms intervals for the servo control (RTC)
    - 2.5ms interval for the servo control pulse (TCB0)
    - 20kHz PWM for motors (TCD)
    - 2 8bit timers in TCA split mode
    - 1 16bit timer in TCB1 
 */
constexpr gpio::Pin NRF_CS_PIN = 13;
constexpr gpio::Pin NRF_RXTX_PIN = 12;
constexpr gpio::Pin NRF_IRQ_PIN = 6;
constexpr gpio::Pin NEOPIXEL_PIN = 7;

constexpr gpio::Pin XL1_PIN = 2;
constexpr gpio::Pin XL2_PIN = 3;
constexpr gpio::Pin XR1_PIN = 4;
constexpr gpio::Pin XR2_PIN = 5;

constexpr gpio::Pin ML1_PIN = 0;
constexpr gpio::Pin ML2_PIN = 10;
constexpr gpio::Pin MR1_PIN = 1;
constexpr gpio::Pin MR2_PIN = 11;

NRF24L01 nrf_{NRF_CS_PIN, NRF_RXTX_PIN};

NeopixelStrip<5> leds{NEOPIXEL_PIN};

struct {
    volatile bool tick : 1;
    uint8_t tickCounter : 2;
    volatile bool radioIrq : 1;
} status_;

enum class OutputKind {
    None,
    DigitalInput, // button, IRQ on the pin
    AnalogInput, // photoresistor, etc, repeated ADC
    DigitalOutput, // on or off
    Servo, // Servo control
    // since these use timer A, there can only be two peripherals of this type attached at once
    Audio, // 50% duty cycle audio 0..8000 Hz
    AnalogOutput, // duty-cycle PWM
};

struct IOOutput {
    gpio::Pin pin;
    OutputKind kind;
    uint8_t value;
};

IOOutput outputs_[4];

/** Motor Control
 
    The idea is to use TCD to control both motors: mirror the A and B waveforms on the C and D outputs respectively and then depending on the desired value do the following:

    - idle = disable WOA and WOC, set gpios to low
    - brake = disable WOA and WOC, set the gpios to high
    - clockwise = enable WOA, disable WOC, set WOC pin to low
    - counter-clockwise = enable WOC, disable WOA, set WOA pin to low
 */
namespace motor {
    void initialize() {
        TCD0.CTRLA = TCD_CLKSEL_20MHZ_gc | TCD_CNTPRES_DIV1_gc | TCD_SYNCPRES_DIV1_gc;
        TCD0.CTRLB = TCD_WGMODE_ONERAMP_gc;
        TCD0.CTRLC = TCD_CMPCSEL_PWMA_gc | TCD_CMPDSEL_PWMB_gc;
        // unlock the protected fault register before writing to it 
        CPU_CCP = CCP_IOREG_gc;
        TCD0.FAULTCTRL = TCD_CMPAEN_bm;
        // set the counters to reset at 1024 for roughly 20kHz 
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPACLR = 1024;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPBCLR = 1024;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPASET = 512;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPBSET = 512;
        // enable the timer
        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
        TCD0.CTRLA |= TCD_ENABLE_bm;
    }
}

/** Servo Control

    One timer is enough for all 4 outputs since the servo expects a short pulse (0.5..2.5ms in the lego servo case) every 20 or so ms. RTC ticks are used to switch between outputs and TCB0 is used to precisely terminate the pulse. 

    TCB0 interrupt simply puts the control pin low to end the servo control pulse.
 
 */
namespace servo {
    void initialize() {
        // initialize TCB0 which we use for the servo pulse timing
        // assuming the clock frequency is 10Mhz, then CCMP should be in the range of 5000 to 25000 for 5 to 2.5ms pulse
        TCB0.CTRLB = TCB_CNTMODE_INT_gc;
        TCB0.INTCTRL = TCB_CAPT_bm;
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc; // | TCB_ENABLE_bm;
    }

    void tick() {
        uint8_t i = status_.tickCounter;
        if (outputs_[i].kind == OutputKind::Servo) {
            TCB0.CTRLA &= ~TCB_ENABLE_bm;
            // currently 1964 .. 9821 for 0.5 to 2.5ms pulse duration
            // TODO the real numbers are incorrect because wrong CLKDIV was used
            //TCB0.CCMP = 1964 + (outputs_[i].value & 0xff) * 31;
            TCB0.CCMP = (1964 * 2) + (outputs_[i].value & 0xff) * 31 * 2;
            gpio::high(outputs_[i].pin);
            TCB0.CNT = 0;
            TCB0.CTRLA |= TCB_ENABLE_bm; 
        }
    }
}

ISR(TCB0_INT_vect) {
    gpio::low(outputs_[status_.tickCounter].pin);
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.CTRLA &= ~TCB_ENABLE_bm;
}

/** PWM and tone generation. 
 
    Since both PWM and audio use TCA, it needs to run off the same clock.
 */
namespace pwm {
    void initialize() {
    }
}

namespace radio {

    void irq() {
        status_.radioIrq = true;
    }

    void initialize() {
        if (! nrf_.initialize("TEST2", "TEST1"))
            leds.fill(Color::Red().withBrightness(32));
        else
            leds.fill(Color::Black());
        leds.update();
        gpio::input(NRF_IRQ_PIN);
        attachInterrupt(digitalPinToInterrupt(NRF_IRQ_PIN), radio::irq, FALLING);
        nrf_.standby();
        nrf_.enableReceiver();
    }


}


void setup() {
    gpio::initialize();
    // flash the white LED to notify we are awake
    leds.fill(Color::White().withBrightness(32));
    leds.update();
    // wait for voltages to stabilize
    cpu::delay_ms(200);
    // initialize internal state
    outputs_[0].pin = XL1_PIN;
    outputs_[1].pin = XL2_PIN;
    outputs_[2].pin = XR1_PIN;
    outputs_[3].pin = XR2_PIN;
    outputs_[0].kind = OutputKind::Servo;
    outputs_[0].value = 255;
    outputs_[1].kind = OutputKind::Servo;
    outputs_[1].value = 255;

    gpio::output(XL1_PIN);
    gpio::output(XR2_PIN);
    gpio::output(ML1_PIN);
    gpio::output(ML2_PIN);
    gpio::output(MR1_PIN);
    gpio::output(MR2_PIN);


    // initialize the peripherals
    spi::initialize();
    i2c::initializeMaster();

    // initialize the RTC timer to to fire every 5 ms, which for 4 outputs multiplexed gives 20ms per input
    RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
    RTC.PER = 164;
    while (RTC.STATUS != 0) {};
    RTC.CTRLA = RTC_RTCEN_bm;

    motor::initialize();
    servo::initialize();
    pwm::initialize();
    radio::initialize();






    DDISP_INITIALIZE();


    DDISP(0,0,"Setup Done");

}

uint8_t x = 0;

void loop() {
    if (RTC.INTFLAGS & RTC_OVF_bm) {
        RTC.INTFLAGS = RTC_OVF_bm;
        //status_.tick = false;
        ++status_.tickCounter;
        servo::tick();
        if (++x % 128 == 0) {
            outputs_[0].value += 1;
        }
    }
    /*
    if (status_.radioIrq) {
        // TODO
    }
    */
    /*
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
    */
}
