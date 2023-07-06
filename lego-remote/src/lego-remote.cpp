#include "platform/platform.h"
#include "platform/peripherals/nrf24l01.h"
#include "platform/peripherals/neopixel.h"

//#include "remote.h"

#include "remote/remote.h"

using namespace platform;
using namespace remote;

/** LEGO Remote
 
    A remote controller to be used mainly with lego. Supports DC motors at 5 or 9V, servo motors, PWM channels, digital and analog inputs, neopixels and tones. 

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

    Motor Channels

    2 motor channels (ML and MR) are supported. Each channel controls a single DC motor with two PWM pins for CW and CCW operation in 64 speed steps and braking & coasting. TCD is used to drive the motors' PWM signals. The motor channels use 3x4 pin connectors where the connector can select whether the pin will use 5V or 9V. The connectors can be inserted either way and have the following pinout:

        5V   +   -   9V
        SEL GND GND SEL
        9V   -   +   5V

    Configurable Channels

    4 extra channels are provided, XL1, XL2 and XR1 and XR2. The channels use a 2x3 connector that provides 5V, GND and one pin that can be configured as either of:

    - digital input
    - analog input (ADC)
    - digital output
    - analog output (PWM)

    Timers

    - 20ms intervals for the servo control (RTC)
    - 2.5ms interval for the servo control pulse (TCB0)
    - 2 8bit timers in TCA split mode
    - 1 16bit timer in TCB1 

 */
class Remote {
public:

    static constexpr gpio::Pin NEOPIXEL_PIN = 7; 

    static constexpr gpio::Pin ML1 = 0; // TCD, WOA
    static constexpr gpio::Pin MR1 = 1; // TCD, WOB
    static constexpr gpio::Pin ML2 = 10; // TCD, WOC
    static constexpr gpio::Pin MR2 = 11; // TCD, WOD

    static constexpr gpio::Pin XL1 = 4; // PB5, ADC0-8, TCA-WO2* (low channel 2)
    static constexpr gpio::Pin XL2 = 5; // PB4, ADC0-9, TCA-WO1* (low channel 1)
    static constexpr gpio::Pin XR1 = 13; // PC3, ADC1-9, TCA-W03 (high channel 0)
    static constexpr gpio::Pin XR2 = 12; // PC2, ADC1-8, uses TCA-W0 (low channel 0) cmp and ovf interrupt to drive the pin

    static constexpr gpio::Pin NRF_CS = 13;
    static constexpr gpio::Pin NRF_RXTX = 12;
    static constexpr gpio::Pin NRF_IRQ = 6;

    static void initialize() {
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.3V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
        cpu::initialize();
        gpio::initialize();
        // set configurable channel pins to input 
        gpio::input(XL1);
        gpio::input(XL2);
        gpio::input(XR1);
        gpio::input(XR2);
        // initialize TCD used to control the two motors, disable prescalers, set one ramp waveform and set WOC to WOA and WOD to WOB. 
        TCD0.CTRLA = TCD_CLKSEL_20MHZ_gc | TCD_CNTPRES_DIV1_gc | TCD_SYNCPRES_DIV1_gc;
        TCD0.CTRLB = TCD_WGMODE_ONERAMP_gc;
        TCD0.CTRLC = TCD_CMPCSEL_PWMA_gc | TCD_CMPDSEL_PWMB_gc;
        // disconnect the pins from the timer
        CPU_CCP = CCP_IOREG_gc;
        TCD0.FAULTCTRL = 0;
        // enable the timer
        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
        TCD0.CTRLA |= TCD_ENABLE_bm;
        // ensure motor pins output low so that any connected motors are floating
        gpio::output(ML1);
        gpio::low(ML1);
        gpio::output(ML2);
        gpio::low(ML2);
        gpio::output(MR1);
        gpio::low(MR1);
        gpio::output(MR2);
        gpio::low(MR2);
        // set the reset counter to 255 for both A and B. This gives us 78.4kHz PWM frequency. It is important for both values to be the same. By setting max to 255 we can simply set 
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPACLR = 127;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPBCLR = 127;
        // initialize the RTC to fire every 5ms which gives us a tick that can be used to switch the servo controls, 4 servos max, multiplexed gives the freuency of updates for each at 20ms
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        RTC.PER = 164;
        while (RTC.STATUS != 0) {};
        RTC.CTRLA = RTC_RTCEN_bm;
        // initialize TCB0 which is used to time the servo control interval precisely
        TCB0.CTRLB = TCB_CNTMODE_INT_gc;
        TCB0.INTCTRL = TCB_CAPT_bm;
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc; // | TCB_ENABLE_bm;
        // initialize TCA for PWM outputs on the configurable channels. We use split mode
        TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm; // enable split mode
        TCA0.SPLIT.CTRLB = 0;    
        //TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0EN_bm | TCA_SPLIT_HCMP0EN_bm; // enable W0 and W3 outputs on pins
        //TCA0.SPLIT.LCMP0 = 64; // backlight at 1/4
        //TCA0.SPLIT.HCMP0 = 128; // rumbler at 1/2
        TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc | TCA_SPLIT_ENABLE_bm; 



        // initialize the NRF radio
        initializeRadio();


        // clear all RGB colors, set the control LED to green & update
        rgbColors_.clear();
        rgbColors_[0] = Color::Green();
        rgbColors_.update();
    }

    static void loop() {
        checkAnalogIn();
        servoTick();
        if (gpio::read(NRF_IRQ))
            radioIrq();
    }


    /** \name Radio comms
     */
    //@{
    static inline NRF24L01 radio_{NRF_CS, NRF_RXTX};
    static inline uint8_t rxBuffer_[32];
    static inline uint8_t txBuffer_[32];
    static inline uint8_t txIndex_ = 0;

    /** Initializes the radio and enters the receiver mode.
     */
    static void initializeRadio() {
        radio_.initialize("AAAAA", "BBBBB");
        radio_.standby();
        radio_.enableReceiver();
    }

    /** Receive command, process it and optionally send a reply.
     */
    static void radioIrq() {
        radio_.clearDataReadyIrq();
        while (radio_.receive(rxBuffer_, 32)) {
            if (rxBuffer_[0] == msg::CommandPacket) {
                switch (static_cast<msg::Kind>(txBuffer_[1])) {
                    case msg::Kind::SetChannelConfig: 
                        setChannelConfig(rxBuffer_[1], rxBuffer_ + 2);
                        break;
                    default:
                        break;
                }
           
            // we are dealing with a channel packet message, parse it into channels and process them one by one 
            } else {
                uint8_t i = 0;
                while (i < 32) {
                    uint8_t channelIndex = rxBuffer_[i];
                    ++i;
                    i += setChannelControl(channelIndex, rxBuffer_ + i);
                }
            }
        }
    }

    static void setChannelConfig(uint8_t channel, uint8_t * data) {
        switch (channel) {
            case 1: // Motor L
                ml_.config = *reinterpret_cast<channel::Motor::Config*>(data);
                break;
            case 2: // Motor R
                mr_.config = *reinterpret_cast<channel::Motor::Config*>(data);
                break;
            case 3: // L1
            case 4: // R1
            case 5: // L2
            case 6: // R2
            case 7: // tone effect
            case 8: //  RGBStrip
            default:
                // error
                break;
        }
    }

    /** Sets channel control for the given channel indes and returns the length of bytes read. 
     */
    static uint8_t setChannelControl(uint8_t channelIndex, uint8_t * data) {
        switch (channelIndex) {
            case 1:
                setMotorL(*reinterpret_cast<channel::Motor::Control*>(data));
                return sizeof(channel::Motor::Control);
            case 2:
                setMotorR(*reinterpret_cast<channel::Motor::Control*>(data));
                return sizeof(channel::Motor::Control);
            /*
            case 3: 
                setChannelL1(*reinterpret_cast<channel::CustomIO::Control*>(data));
                return sizeof(channel::CustomIO::Control);
            */
            default:
                // TODO ERROR
                // return 32 which will make processing any further channels that might have been in the message impossible as it overflows the rxBuffer  
                return 32; 
                
        }
    }

    //@}

    /** \name Channels
     
     */
    //@{
    static inline channel::Motor ml_; // 1
    static inline channel::Motor mr_; // 2
    static inline channel::CustomIO l1_; // 3
    static inline channel::CustomIO r1_; // 4
    static inline channel::CustomIO l2_; // 5
    static inline channel::CustomIO r2_; // 6
    static inline channel::ToneEffect tone_; // 7
    static inline channel::RGBStrip rgb_; // 8

    // channels 8 .. 15 are colors actually, the first LED in the strip is the control led on the device
    static inline NeopixelStrip<9> rgbColors_{NEOPIXEL_PIN};
    //@}

    /** \name Motor Control

        To engage break, disconnect the timer from both pins and set both to high. To let the motor coast, disconnect the timer from both pins and set both to low. The forward / backward operation is enabled by directing the PWM output to one of the pins only.

     */
    //{@

    static void setMotorL(channel::Motor::Control const & ctrl) {
        if (ctrl != ml_.control) {
            switch (ctrl.mode) {
                case channel::Motor::Mode::Brake:
                    if (ml_.isSpinning()) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPAEN_bm | TCD_CMPCEN_bm);
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    };
                    gpio::high(ML1);
                    gpio::high(ML2);
                    break;
                case channel::Motor::Mode::Coast:
                    if (ml_.isSpinning()) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPAEN_bm | TCD_CMPCEN_bm);
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    gpio::low(ML1);
                    gpio::low(ML2);
                    break;
                case channel::Motor::Mode::CW:
                    while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
                    TCD0.CMPASET = 255 - ctrl.speed;
                    if (ml_.control.mode != channel::Motor::Mode::CW) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPAEN_bm | TCD_CMPCEN_bm);
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL |= TCD_CMPAEN_bm;
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    break;
                case channel::Motor::Mode::CCW:
                    while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
                    TCD0.CMPASET = 255 - ctrl.speed;
                    if (ml_.control.mode != channel::Motor::Mode::CCW) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPAEN_bm | TCD_CMPCEN_bm);
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL |= TCD_CMPCEN_bm;
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    break;
            }
            ml_.control = ctrl;
        }
    }

    static void setMotorR(channel::Motor::Control const & ctrl) {
        if (ctrl != mr_.control) {
            switch (ctrl.mode) {
                case channel::Motor::Mode::Brake:
                    if (mr_.isSpinning()) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPBEN_bm | TCD_CMPDEN_bm);
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    gpio::high(MR1);
                    gpio::high(MR2);
                    break;
                case channel::Motor::Mode::Coast:
                    if (mr_.isSpinning()) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPBEN_bm | TCD_CMPDEN_bm);
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    gpio::low(MR1);
                    gpio::low(MR2);
                    break;
                case channel::Motor::Mode::CW:
                    while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
                    TCD0.CMPBSET = 255 - ctrl.speed;
                    if (mr_.control.mode != channel::Motor::Mode::CW) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPBEN_bm | TCD_CMPDEN_bm);
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL |= TCD_CMPBEN_bm;
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    break;
                case channel::Motor::Mode::CCW:
                    while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
                    TCD0.CMPBSET = 255 - ctrl.speed;
                    if (mr_.control.mode != channel::Motor::Mode::CCW) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPBEN_bm | TCD_CMPDEN_bm);
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL |= TCD_CMPDEN_bm;
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    break;
            }
            mr_.control = ctrl;
        }

    }

    //@}
    

    /** \name Servo Control 
     
        Uses TCB0 to control any of the Custom IO channels that are set in the servo mode using the 5ms RTC interrupt. Since each servo needs an update only once every ~20ms, all four channels are multiplexed. 

        TODO figure out decent values for the servos
     */
    //@{
    static inline uint8_t activeServoPin_;
    static inline uint8_t servoTick_ = 0;

    static void servoTick() {
        servoTick_ = (servoTick_ + 1) % 4;
        switch (servoTick_) {
            case 0: 
                startControlPulse(l1_, XL1);
                break;
            case 1:
                startControlPulse(l2_, XL2);
                break;
            case 2:
                startControlPulse(r1_, XR1);
                break;
            case 3:
                startControlPulse(r1_, XR1);
                break;
        }
    }

    static void startControlPulse(channel::CustomIO const & ch, gpio::Pin pin) {
        if (ch.config.mode != channel::CustomIO::Mode::Servo)
            return;
        activeServoPin_= pin;
        // calculate the pulse duration from the config
        TCB0.CTRLA &= ~TCB_ENABLE_bm;
        TCB0.CCMP = (ch.config.servoEnd - ch.config.servoStart) * ch.control.value / 255 + ch.config.servoStart;
        gpio::high(activeServoPin_);
        TCB0.CNT = 0;
        TCB0.CTRLA |= TCB_ENABLE_bm; 
    }

    static void terminateControlPulse() {
        gpio::low(activeServoPin_);
        TCB0.INTFLAGS = TCB_CAPT_bm;
        TCB0.CTRLA &= ~TCB_ENABLE_bm;
    }
    //}@

    /** \name Analog input 
     
        Uses the ADC to read the analog inputs, if any. 
     */
    //@{
    static void checkAnalogIn() {
        if (! (ADC0.INTFLAGS & ADC_RESRDY_bm))
            return false;
        uint8_t muxpos = ADC0.MUXPOS;
        switch (muxpos) {
            case ADC_MUXPOS_TEMPSENSE_gc:
                // TODO calculate temperature
                // switch to VCC as reference for the custom pins
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; 
                if (l1_.config.mode == channel::CustomIO::Mode::AnalogIn) {
                    break;
                }
                // fallthrough to next 
            
            default:
                ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
                ADC0.SAMPCTRL = 31;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm; // internal reference
                break;
        }
    }
    //}@

}; // Remote

/** The TCB only fires when the currently multiplexed output is a servo motor. When it fires, we first pull the output low to terminate the control pulse and then disable the timer.
 */
ISR(TCB0_INT_vect) {
    Remote::terminateControlPulse();
}

void setup() {
    Remote::initialize();
}

void loop() {
    Remote::loop();
}




#ifdef FOOBAR



class Remote {
public:

    static constexpr gpio::Pin NEOPIXEL_PIN = 7; 

    static constexpr gpio::Pin ML1 = 0; // TCD, WOA
    static constexpr gpio::Pin MR1 = 1; // TCD, WOB
    static constexpr gpio::Pin ML2 = 10; // TCD, WOC
    static constexpr gpio::Pin MR2 = 11; // TCD, WOD

    static constexpr gpio::Pin XL1 = 4; // PB5, ADC0-8, TCA-WO2* (low channel 2)
    static constexpr gpio::Pin XL2 = 5; // PB4, ADC0-9, TCA-WO1* (low channel 1)
    static constexpr gpio::Pin XR1 = 13; // PC3, ADC1-9, TCA-W03 (high channel 0)
    static constexpr gpio::Pin XR2 = 12; // PC2, ADC1-8, uses TCA-W0 (low channel 0) cmp and ovf interrupt to drive the pin

    // PA6 ADC0-6, ADC1-2
    // PA7 ADC0-7, ADC1-3
    // PB5 ADC0-8, TCA-WO2* -----
    // PB4 ADC0-9, TCA-WO1* -----
    // PB3 TCA-WO0 
    // PB2 TCA-WO2
    // PC3 ADC1-9, TCA-W03 ------
    // PC2 ADC1-8, will use TCA-W0 interrupt to change pin value 
    // 

    static void initialize() {
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.3V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
        // set configurable channel pins to input 
        gpio::input(XL1);
        gpio::input(XL2);
        gpio::input(XR1);
        gpio::input(XR2);
        // initialize TCD used to control the two motors, disable prescalers, set one ramp waveform and set WOC to WOA and WOD to WOB. 
        TCD0.CTRLA = TCD_CLKSEL_20MHZ_gc | TCD_CNTPRES_DIV1_gc | TCD_SYNCPRES_DIV1_gc;
        TCD0.CTRLB = TCD_WGMODE_ONERAMP_gc;
        TCD0.CTRLC = TCD_CMPCSEL_PWMA_gc | TCD_CMPDSEL_PWMB_gc;
        // disconnect the pins from the timer
        CPU_CCP = CCP_IOREG_gc;
        TCD0.FAULTCTRL = 0;
        // enable the timer
        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
        TCD0.CTRLA |= TCD_ENABLE_bm;
        // ensure motor pins output low so that any connected motors are floating
        gpio::output(ML1);
        gpio::low(ML1);
        gpio::output(ML2);
        gpio::low(ML2);
        gpio::output(MR1);
        gpio::low(MR1);
        gpio::output(MR2);
        gpio::low(MR2);
        // set the reset counter to 255 for both A and B. This gives us 78.4kHz PWM frequency. It is important for both values to be the same. By setting max to 255 we can simply set 
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPACLR = 127;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPBCLR = 127;
        // initialize the RTC to fire every 5ms which gives us a tick that can be used to switch the servo controls, 4 servos max, multiplexed gives the freuency of updates for each at 20ms
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        RTC.PER = 164;
        while (RTC.STATUS != 0) {};
        RTC.CTRLA = RTC_RTCEN_bm;
        // initialize TCB0 which is used to time the servo control interval precisely
        TCB0.CTRLB = TCB_CNTMODE_INT_gc;
        TCB0.INTCTRL = TCB_CAPT_bm;
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc; // | TCB_ENABLE_bm;
        // initialize TCA for PWM outputs on the configurable channels. We use split mode
        TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm; // enable split mode
        TCA0.SPLIT.CTRLB = 0;    
        //TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0EN_bm | TCA_SPLIT_HCMP0EN_bm; // enable W0 and W3 outputs on pins
        //TCA0.SPLIT.LCMP0 = 64; // backlight at 1/4
        //TCA0.SPLIT.HCMP0 = 128; // rumbler at 1/2
        TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc | TCA_SPLIT_ENABLE_bm; 


        // clear all RGB colors, set the control LED to green & update
        rgbColors_.clear();
        rgbColors_[0] = Color::Green();
        rgbColors_.update();


        //motorCW(0, 4);
        //motorCW(0, 144);
        //motorCoast(0);
    }


    /** \name Channels 

        Channels:

        01 - Motor L
        02 - Motor R
        03 - Custom L1
        04 - Custom L2
        05 - Custom R1
        06 - Custom R2
        07 - Tone Effect Generator 
        08 - RGBStrip
        09 - Color 1
        10 - Color 2
        11 - Color 3
        12 - Color 4
        13 - Color 5
        14 - Color 6
        15 - Color 7
        16 - Color 8

     */
    //@{
    static inline remote::MotorChannel ml_;
    static inline remote::MotorChannel mr_;
    static inline remote::CustomIOChannel l1_;
    static inline remote::CustomIOChannel l2_;
    static inline remote::CustomIOChannel r1_;
    static inline remote::CustomIOChannel r2_;
    static inline remote::ToneEffectChannel tone_;
    static inline remote::RGBStripChannel rgb_;

    // channels 8 .. 15 are colors actually, the first LED in the strip is the control led on the device
    static inline NeopixelStrip<9> rgbColors_{NEOPIXEL_PIN};
    //@}

    /** \name Servo Motors
     */
    //@{

    static inline gpio::Pin activeServoPin_;

    static void servoTick() {
        // if the index'th output is servo, then calculate the duration of the pulse 
        uint8_t value = 67;
        TCB0.CCMP = 5000 + (157 * 67) / 2; 
        //activeServoPin_ = pin;
        TCB0.CNT = 0;
        //gpio::high(pin);
        TCB0.CTRLA |= TCB_ENABLE_bm;
    }

    //@}

}; // Remote

/** The TCB only fires when the currently multiplexed output is a servo motor. When it fires, we first pull the output low to terminate the control pulse and then disable the timer.
 */
ISR(TCB0_INT_vect) {
    gpio::low(Remote::activeServoPin_);
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.CTRLA &= ~TCB_ENABLE_bm;
}


void setup() {
    Remote::initialize();
}

void loop() {

}



#ifdef OLD
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
        TCD0.CMPACLR = 256;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPBCLR = 256;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPASET = 128;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPBSET = 128;
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
    cpu::delayMs(200);
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


#endif
#endif