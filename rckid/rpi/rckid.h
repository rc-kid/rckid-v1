#pragma once

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-uinput.h"

#include "platform/platform.h"
#include "peripherals/nrf24l01.h"
#include "peripherals/mpu6050.h"

#include "common/config.h"
#include "common/comms.h"
#include "events.h"

// TODO change log and error to something more meaningful

#include <iostream>
#define LOG(...) std::cout << __VA_ARGS__ << std::endl
#define ERROR(...) std::cout << "ERROR:" << __VA_ARGS__ << std::endl

/** RCKid RPI Driver

                           3V3     5V
                I2C SDA -- 2       5V
                I2C SCL -- 3      GND
                NRF_IRQ -- 4*      14 -- NRF_RXTX / UART TX
                           GND     15 -- BTN_X / UART RX
                  BTN_B -- 17      18 -- BTN_A
            DISPLAY_RST -- 27*    GND
             DISPLAY_DC -- 22*    *23 -- BTN_Y (voldown)
                           3V3    *24 -- RPI_POWEROFF
    DISPLAY MOSI (SPI0) -- 10     GND
    DISPLAY MISO (SPI0) -- 9      *25 -- AVR_IRQ
    DISPLAY SCLK (SPI0) -- 11       8 -- DISPLAY CE (SPI0 CE0)
                           GND      7 -- BTN_L
                  BTN_R -- 0        1 -- BTN_RVOL (volup)
            ()   BTN_LVOL -- 5*     GND
             HEADPHONES -- 6*      12 -- AUDIO L
                AUDIO R -- 13     GND
              SPI1 MISO -- 19      16 -- SPI1 CE0
                JOY_BTN -- 26      20 -- SPI1 MOSI
                           GND     21 -- SPI1 SCLK

*/

#define PIN_AVR_IRQ 25
#define PIN_HEADPHONES 6
#define PIN_NRF_CS 16
#define PIN_NRF_RXTX 14
#define PIN_NRF_IRQ 4

#define PIN_BTN_A 18
#define PIN_BTN_B 17
#define PIN_BTN_X 15
#define PIN_BTN_Y 23
#define PIN_BTN_L 7
#define PIN_BTN_R 0
#define PIN_BTN_LVOL 5
#define PIN_BTN_RVOL 1

#define MAIN_THREAD
#define DRIVER_THREAD
#define ISR_THREAD

/** RCKid Driver
 
 */
class RCKid {
public:

    static constexpr char const * LIBEVDEV_DEVICE_NAME = "RCKid-gamepad";

    static constexpr uint8_t BTN_DEBOUNCE_DURATION = 2;

    static constexpr uint8_t BTN_AUTOREPEAT_DURATION = 50;

    /** Initializes an RC boy instance and returns it. 
     
        The initializer starts the hw loop and initializes the libevdev gamepad layer. 
     */
    static RCKid * initialize() MAIN_THREAD; 

    static RCKid * instance(); 


private:

    /** A digital button. 
     
        We keep the current state of the button as set by the ISR and the reported state of the button, which is the last state that has been reported to the driver and ui thread. 

        When the ISR of the button's pin is activated, the current state is set accordingly and if the button is not being debounced now (debounce counter is 0), the action is taken. This means that the debounce counter is reset to its max value (BTN_DEBOUNCE_PERIOD) and the libevdev device and main thread are notified of the change. The driver's thread monitors all buttons on every tick and decreases their debounce counters. When they reach zero, we check if the current state is different than reported and if so, initiate button action. This gives the best responsivity while still debouncing. 

        Autorepeat is also handled by the ticks in the driver's thread. 

        The debounce and autorepeat counters are atomically updated (aligned addresses & unsigned should be atomic on ARM) and their values and their handling ensure that the driver thread and ISR thread will not interfere. 

        The volume buttons and the thumbstick button are already debounced on the AVR to reduce the I2C talk to minimum. 
     */
    struct Button {
        bool current = false ISR_THREAD; 
        bool reported = false ISR_THREAD DRIVER_THREAD;
        unsigned debounce = 0 ISR_THREAD DRIVER_THREAD;
        unsigned autorepeat = 0 ISR_THREAD DRIVER_THREAD;
        unsigned const evdevId;

        /** Creates new button, parametrized by the pin and its evdev id. 
         */
        Button(unsigned evdevId): evdevId{evdevId} { }

    }; // RCKid::Button

    /** An analog axis.

        We use the analog axes for the thumbstick and accelerometer. The thumbstick is debounced on the AVR.   
     */
    struct Axis {
        unsigned current;
        unsigned const evdevId;

        Axis(unsigned evdevId):evdevId{evdevId} { }
    }; // RCKid:Axis

    /** Event for the driver's main loop to react to. Events with specified numbers are changes on the specified pins.
    */
    enum class HWEvent {
        Tick,
        AvrIrq = PIN_AVR_IRQ, 
        Headphones = PIN_HEADPHONES, 
        ButtonA = PIN_BTN_A, 
        ButtonB = PIN_BTN_B, 
        ButtonX = PIN_BTN_X, 
        ButtonY = PIN_BTN_Y, 
        ButtonL = PIN_BTN_L, 
        ButtonR = PIN_BTN_R, 
        ButtonLVol = PIN_BTN_LVOL, 
        ButtonRVol = PIN_BTN_RVOL, 
        NrfIrq = PIN_NRF_IRQ, 

    }; // Driver::Event


    /** Initializes the RCKid manager. 
     */
    RCKid() MAIN_THREAD;

    /** The HW loop, proceses events from the hw event queue. This method is executed in a separate thread, which isthe only thread that accesses the GPIOs and i2c/spi connections. 
     */
    void hwLoop() DRIVER_THREAD;

    /** Queries the accelerometer status and updates the X and Y accel axes. 
     */
    void accelQueryStatus() DRIVER_THREAD;

    /** Requests the fast AVR status update and processes the result.
     */
    void avrQueryState() DRIVER_THREAD;

    /** Requests the full AVR state update and processes the result. 
     */
    void avrQueryExtendedState() DRIVER_THREAD;

    /** Processes the avr status. 
     */
    void processAvrStatus(comms::Status const & status) DRIVER_THREAD;

    /** Processes the controls information sent by the AVR (buttons, thumbstick).
     */
    void processAvrControls(comms::Controls const & controls) DRIVER_THREAD;

    /** Initializes the ISRs on the rpi pins so that the driver can respond properly.
     */
    void initializeISRs() MAIN_THREAD;

    /** Initializes the libevdev gamepad device for other applications. 
     */
    void initializeLibevdevGamepad() MAIN_THREAD;

    void initializeAvr() MAIN_THREAD;

    void initializeAccel() MAIN_THREAD;

    void initializeNrf() MAIN_THREAD;

    /** Transmits the given command to the AVR. 
     */
    template<typename T>
    void sendAvrCommand(T const & cmd) DRIVER_THREAD {
        using namespace platform;
        i2c::transmit(AVR_I2C_ADDRESS, reinterpret_cast<uint8_t const *>(& cmd), sizeof(T), nullptr, 0);
    }

    static void isrAvrIrq() ISR_THREAD {
        RCKid * self = RCKid::instance();
        self->hwEvents_.send(HWEvent::AvrIrq);
    }

    static void isrNrfIrq() ISR_THREAD {
        RCKid * self = RCKid::instance();
        self->hwEvents_.send(HWEvent::NrfIrq);
    }

    static void isrHeadphones() {
        // TODO
    }

    static void isrButtonA() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_A), i->btnA_);
    }

    static void isrButtonB() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_B), i->btnB_);
    }

    static void isrButtonX() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_X), i->btnX_);
    }

    static void isrButtonY() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_Y), i->btnY_);
    }

    static void isrButtonL() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_L), i->btnL_);
    }

    static void isrButtonR() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_R), i->btnR_);
    }

    static void isrButtonLVol() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_LVOL), i->btnVolDown_);
    }

    static void isrButtonRVol() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_RVOL), i->btnVolUp_);
    }

    /** ISR for the hardware buttons and the headphones. 
     
        Together with the driver thread's ticks, the isr is responsibel for debouncing. Called by the ISR or driver thread depending on the button. 
     */
    void buttonChange(int level, Button & btn) ISR_THREAD DRIVER_THREAD {
        // always set the state
        btn.current = ! level;
        // if debounce is 0, take action and set the debounce timer, otherwise do nothing
        if (btn.debounce == 0) {
            btn.debounce = BTN_DEBOUNCE_DURATION;
            buttonAction(btn);
        }
    }

    void buttonAction(Button & btn) ISR_THREAD DRIVER_THREAD {
        btn.reported = btn.current;
        btn.autorepeat = BTN_AUTOREPEAT_DURATION;
        if (uidev_ != nullptr) {
            libevdev_uinput_write_event(uidev_, EV_KEY, btn.evdevId, btn.reported ? 1 : 0);
            libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
        }
        // TODO send the appropriate action to the main thread
    }

    void buttonTick(Button & btn) DRIVER_THREAD {
        if (btn.debounce > 0 && --(btn.debounce) == 0)
            if (btn.reported != btn.current)
                buttonAction(btn);
        if (btn.reported && btn.autorepeat > 0 && --(btn.autorepeat) == 0)
            buttonAction(btn);
    }

    void axisChange(uint8_t value, Axis & axis) DRIVER_THREAD {
        if (axis.current != value) {
            axis.current = value;
            if (uidev_ != nullptr) {
                libevdev_uinput_write_event(uidev_, EV_ABS, axis.evdevId, value);
                libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
            }
        }
    }

    /** Hardware events sent to the driver's main loop. 
     */
    EventQueue<HWEvent> hwEvents_;

    /** The button state objects. 
     */
    Button btnVolDown_{KEY_VOLUMEDOWN};
    Button btnVolUp_{KEY_VOLUMEUP};
    Button btnJoy_{BTN_JOYSTICK};
    Button btnA_{BTN_A};
    Button btnB_{BTN_B}; 
    Button btnX_{BTN_X};
    Button btnY_{BTN_Y};
    Button btnL_{BTN_LEFT};
    Button btnR_{BTN_RIGHT};
    Button btnSelect_{BTN_SELECT};
    Button btnStart_{BTN_START};
    Button btnHome_{BTN_MODE};
    Button btnDpadUp_{BTN_DPAD_UP};
    Button btnDpadDown_{BTN_DPAD_DOWN};
    Button btnDpadLeft_{BTN_DPAD_LEFT};
    Button btnDpadRight_{BTN_DPAD_RIGHT};

    /** Axes. 
     */
    Axis thumbX_{ABS_RX};
    Axis thumbY_{ABS_RY};
    Axis accelX_{ABS_X};
    Axis accelY_{ABS_Y};

    platform::NRF24L01 radio_{PIN_NRF_CS, PIN_NRF_RXTX};
    platform::MPU6050 accel_;

    /** The libevdev device handle. 
     */
    static inline struct libevdev_uinput * uidev_{nullptr};

}; // RCKid

inline RCKid * RCKid::instance() {
    static RCKid * instance = new RCKid{};
    return instance;
} 
