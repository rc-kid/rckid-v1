#include "platform/platform.h"
#include "platform/peripherals/nrf24l01.h"
#include "platform/peripherals/ssd1306.h"

using namespace platform;

/** Chip Pinout
               -- VDD             GND --
         DEBUG -- (00) PA4   PA3 (16) -- SCK
               -- (01) PA5   PA2 (15) -- MISO
        NRF_CS -- (02) PA6   PA1 (14) -- MOSI
      NRF_RXTX -- (03) PA7   PA0 (17) -- UPDI
               -- (04) PB5   PC3 (13) -- 
               -- (05) PB4   PC2 (12) -- 
       NRF_IRQ -- (06) PB3   PC1 (11) -- 
               -- (07) PB2   PC0 (10) -- 
           SDA -- (08) PB1   PB0 (09) -- SCL
 */
class Repeater {
public:

    static constexpr gpio::Pin NRF_CS_PIN = 2;
    static constexpr gpio::Pin NRF_RXTX_PIN = 3;
    static constexpr gpio::Pin NRF_IRQ_PIN = 6;

    static constexpr gpio::Pin DEBUG_PIN = 0;

    static inline NRF24L01 nrf_{NRF_CS_PIN, NRF_RXTX_PIN};
    static inline SSD1306 oled_;
    static inline uint16_t msgs_;
    static inline uint16_t msgsNow_;
    static inline uint16_t msgsLastSec_;
    static inline uint8_t buffer_[32];
    static inline uint16_t errors_ = 0;


    static void initialize() {
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.3V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
        //cpu::delayMs(100);
        // initialize basic peripherals
        gpio::initialize();
        spi::initialize();
        i2c::initializeMaster();

        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR, also start the timer
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        //RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
        RTC.PITCTRLA = RTC_PERIOD_CYC512_gc | RTC_PITEN_bm;

        // initialize the OLED display
        oled_.initialize128x32();
        oled_.normalMode();
        oled_.clear32();
        oled_.write(0,0,"NRF REPEATER");
        oled_.write(0,2, "Total:");
        oled_.write(0,3, "Last:");
        oled_.write(64, 2, "Errors:");
        // 3684
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc; // | TCB_ENABLE_bm;
        TCB0.INTCTRL = TCB_CAPT_bm;



        if (nrf_.initialize("AAAAA", "AAAAA", 86)) 
           oled_.write(64, 0, "NRF OK");
        else 
            oled_.write(64, 0, "NRF FAIL"); 
        nrf_.standby();
        nrf_.enableReceiver();
        gpio::inputPullup(5);
        gpio::output(4);
        gpio::high(4);
    }

    static void myTone(uint16_t freq) {
        TCB0.CCMP = 2500000 / freq;
        TCB0.CTRLA |= TCB_ENABLE_bm;
    }

    static void myNoTone() {
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc;
        gpio::high(4);
    }

    static inline uint16_t freq = 0;
    static inline int8_t siren_ = 0;
    static inline bool xx ;

    static void loop() {
        if (gpio::read(5)) {
            if (freq != 0) {
                freq = 0;
                myNoTone();
            }
        } else {
            if (freq == 0) {
                freq = 600;
                myTone(freq);
            }
        }
        if (gpio::read(NRF_IRQ_PIN) == 0) {
            NRF24L01::Status status = nrf_.getStatus();
            if (transmitting_) {
                if (status.txDataSentIrq() || status.txDataFailIrq()) {
                    transmitting_ = false;
                    nrf_.enableReceiver();
                }
            }
            if (status.txDataFailIrq())
                ++errors_;
            nrf_.clearIrq();
            while (nrf_.receive(buffer_, 32)) {
                ++msgs_;
                ++msgsNow_;
            }
        }
        if (RTC.PITINTFLAGS == RTC_PI_bm) {
            RTC.PITINTFLAGS = RTC_PI_bm;
            if (gpio::read(DEBUG_PIN))
                gpio::low(DEBUG_PIN);
            else 
                gpio::high(DEBUG_PIN);
            oled_.write(35,2, msgs_);
            oled_.write(35, 3, heartbeatIndex_);
            oled_.write(80,2, errors_);
            // send the heartbeat
            //walkieTalkieHeartbeat();
            if (freq != 0) {
                if (siren_) {
                    freq += 3;
                    if (freq >= 1200) {
                        freq = 1200;
                        siren_ = 0;
                    }
                } else {
                    freq -= 3;
                    if (freq <= 600) {
                        freq = 600;
                        siren_ = 1;
                    }
                }
            } 

            /* hi-low 0.5s
             if (freq == 450) {
                freq = 600;
            } else {
                if (freq == 600)
                    freq = 450;
            } 
            */
            if (freq != 0)
                myTone(freq);
        }

    }

    static void walkieTalkieHeartbeat() {
        uint8_t packet[32];
        packet[0] = 0x80; // heartbeat id
        packet[1] = heartbeatIndex_++;
        packet[2] = 12;
        memcpy(packet + 3, "NRF_REPEATER", 12);
        nrf_.standby();
        nrf_.transmit(packet, 32);
        nrf_.enableTransmitter();
        transmitting_ = true;
    }

    static inline uint8_t heartbeatIndex_ = 0;
    static inline bool transmitting_ = false;



}; // Repeater

uint8_t * addr = PORTB.OUTTGL;
uint8_t pins = 1 << 5;

// 3404
ISR(TCB0_INT_vect) {
    TCB0.INTFLAGS = TCB_CAPT_bm;
    PORTB.OUTTGL = 1 << 5;
    //*addr = pins;
    /*
    Repeater::xx ? gpio::high(4) : gpio::low(4);
    Repeater::xx = !Repeater::xx;
    */
}

void setup() {
    Repeater::initialize();
}

void loop() {
    Repeater::loop();
}

#ifdef FOOBAR


// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1


volatile bool nrf_irq = false;
volatile bool lora_irq = false;
volatile bool tick = false;
uint8_t ticks = 0;
uint16_t nrf_received = 0;
uint16_t nrf_errors = 0;
uint16_t lora_received = 0;
uint16_t lora_errors = 0;
uint8_t nrf_x = 0;
uint8_t lora_x = 0;
bool tickMark = false;

void nrfIrq() {
    nrf_irq = true;
    //gpio::high(DEBUG_PIN);
}

void loraIrq() {
    lora_irq = true;
}

ISR(TCB0_INT_vect) {
    TCB0.INTFLAGS = TCB_CAPT_bm;
    if (++ticks >= 100) {
        tick = true;
        ticks -= 100;
    }
}

void setup() {
    cpu::delayMs(100);
    gpio::initialize();
    spi::initialize();
    i2c::initializeMaster();
  
    oled.initialize128x32();
    oled.normalMode();
    oled.clear32();

    gpio::output(DEBUG_PIN);
    gpio::low(DEBUG_PIN);

    if (! nrf.initialize("TEST2", "TEST1"))
        oled.write(0,0," NRF FAIL");
    else
        oled.write(0,0," NRF OK");
    if (! lora.initialize()) 
        oled.write(64, 0, "LORA_FAIL");
    else
        oled.write(64,0, "LORA OK");
    gpio::input(NRF_IRQ);
    gpio::input(LORA_IRQ);
    attachInterrupt(digitalPinToInterrupt(NRF_IRQ), nrfIrq, FALLING);
    attachInterrupt(digitalPinToInterrupt(LORA_IRQ), loraIrq, RISING);

    nrf.standby();
    nrf.enableReceiver();

    lora.standby();
    lora.enableReceiver();

    // start the 1kHz timer for ticks
    // TODO change this to 8kHz for audio recording, or use different timer? 
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CCMP = 50000; // for 100Hz    
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
}

void loop() {
    if (lora_irq) {
        lora_irq = false;
        lora.clearIrq();
        uint8_t msg[32];
        lora.receive(msg, 32);
        lora_received += 1;
        while (++lora_x != msg[0])
            ++lora_errors;
    }

    if (nrf_irq) {
        nrf_irq = false;
        nrf.clearDataReadyIrq();
        uint8_t msg[32];
        while (nrf.receive(msg, 32)) {
            nrf_received += 1;
            while (++nrf_x != msg[0])
                ++nrf_errors;
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
        oled.write(nrf_received, ' ');
        oled.gotoXY(0,2);
        oled.write(nrf_errors, ' ');
        /*
        oled.gotoXY(0,2);
        oled.write(nrf.getStatus().raw, ' ');
        oled.gotoXY(50,2);
        oled.write(nrf.readRegister(NRF24L01::CONFIG), ' ');
        */
        nrf_received = 0;
        nrf_errors = 0;


        oled.gotoXY(64,1);
        oled.write(lora_received, ' ');
        oled.gotoXY(64, 2);
        oled.write(lora_errors, ' ');
        oled.gotoXY(64, 3);
        oled.write(lora.packetRssi(), ' ');

        lora_received = 0;
        lora_errors = 0;
    }
}


#endif