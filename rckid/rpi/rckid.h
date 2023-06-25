#pragma once

#include <queue>
#include <mutex>
#include <variant>
#include <functional>

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-uinput.h"

#include "platform/platform.h"
#include "platform/peripherals/nrf24l01.h"
#include "platform/peripherals/mpu6050.h"
#include "platform/color.h"

#include "common/config.h"
#include "common/comms.h"
#include "events.h"

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
                BTN_JOY -- 26      20 -- SPI1 MOSI
                           GND     21 -- SPI1 SCLK

*/

#define PIN_AVR_IRQ RPI_PIN_AVR_IRQ
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
#define PIN_BTN_JOY 26


#define MAIN_THREAD
#define DRIVER_THREAD
#define ISR_THREAD

class Window; 

/** RCKid Driver
 
    // pause = A
    // load state = X
    // save state = Y
    // 
 */
class RCKid {
public:

    static constexpr unsigned int RETROARCH_HOTKEY_ENABLE = BTN_THUMBR;
    static constexpr unsigned int RETROARCH_HOTKEY_PAUSE = BTN_SOUTH; // B
    static constexpr unsigned int RETROARCH_HOTKEY_SAVE_STATE = BTN_NORTH; // A
    static constexpr unsigned int RETROARCH_HOTKEY_LOAD_STATE = BTN_WEST; // X
    static constexpr unsigned int RETROATCH_HOTKEY_SCREENSHOT = BTN_EAST; // Y
    
    static constexpr char const * LIBEVDEV_GAMEPAD_NAME = "rckid-gamepad";
    static constexpr char const * LIBEVDEV_KEYBOARD_NAME = "rckid-keyboard";

    static constexpr uint8_t BTN_DEBOUNCE_DURATION = 2;

    static constexpr uint8_t BTN_AUTOREPEAT_DURATION = 20;

    /** Initializes the RCKid driver. 

        The initializer starts the hw loop and initializes the libevdev gamepad layer. 
     */
    RCKid(Window * window) MAIN_THREAD;

    ~RCKid() {
        libevdev_uinput_destroy(gamepad_);
        libevdev_free(gamepadDev_);
    }

    /** Enables or disables automatic sending of button & analog events to the virtual gamepad. 
     */
    void enableGamepad(bool enable) {
        hwEvents_.send(EnableGamepad{enable});
    }

    void keyPress(int key, bool state) {
        hwEvents_.send(KeyPress{key, state});
    }

    void powerOff() {
        hwEvents_.send(msg::PowerDown{});
    }

    /** Returns the current audio volume. 
     */
    int volume() const { return status_.volume; }

    /** Sets the current audio volume
     */
    void setVolume(int value) {
        if (value < 0)
            value = 0;
        if (value > AUDIO_MAX_VOLUME)
            value = AUDIO_MAX_VOLUME;
        status_.changed = true;
        status_.volume = value;
        system(STR("amixer sset -q Headphone -M " << status_.volume << "%").c_str());
    }

    uint8_t brightness() const { return status_.brightness; }

    void setBrightness(uint8_t value) { 
        status_.changed = true;
        status_.brightness = value; hwEvents_.send(msg::SetBrightness{value}); 
    }

    void rgbOn() { hwEvents_.send(msg::RGBOn{}); }
    void rgbOff() { hwEvents_.send(msg::RGBOff{}); }
    void rgbColor(platform::Color color) { hwEvents_.send(msg::RGBColor{color}); }

    void startRecording(std::function<void(RecordingEvent &)> callback) {
        TraceLog(LOG_DEBUG, "Recording start");
        recordingCallback_ = callback;
        status_.recording = true;
        hwEvents_.send(msg::StartAudioRecording{});
    }

    void stopRecording() { 
        TraceLog(LOG_DEBUG, "Recording stopped");
        hwEvents_.send(msg::StopAudioRecording{}); 
        status_.recording = false; 
    }

    /** Returns true if the device status information changed since the last call. 
     */
    bool statusChanged() {
        if (status_.changed) {
            status_.changed  = false;
            return true;
        } else {
            return false;
        }
    }

    comms::Mode mode() const { return status_.mode; }
    bool usb() const { return status_.usb; }
    bool charging() const { return status_.charging; }
    uint16_t vBatt() const { return status_.vBatt; }
    uint16_t vcc() const { return status_.vcc; }
    int16_t avrTemp() const { return status_.avrTemp; }
    int16_t accelTemp() const { return status_.accelTemp; }
    bool headphones() const { return status_.headphones; }
    bool wifi() const { return status_.wifi; }
    bool wifiHotspot() const { return status_.wifiHotspot; }
    std::string const & ssid() const { return status_.ssid; }
    bool nrf() const { return status_.nrf; }

private:

    friend class Window;

    /** A digital button. 
     
        We keep the current state of the button as set by the ISR and the reported state of the button, which is the last state that has been reported to the driver and ui thread. 

        When the ISR of the button's pin is activated, the current state is set accordingly and if the button is not being debounced now (debounce counter is 0), the action is taken. This means that the debounce counter is reset to its max value (BTN_DEBOUNCE_PERIOD) and the libevdev device and main thread are notified of the change. The driver's thread monitors all buttons on every tick and decreases their debounce counters. When they reach zero, we check if the current state is different than reported and if so, initiate button action. This gives the best responsivity while still debouncing. 

        Autorepeat is also handled by the ticks in the driver's thread. 

        The debounce and autorepeat counters are atomically updated (aligned addresses & unsigned should be atomic on ARM) and their values and their handling ensure that the driver thread and ISR thread will not interfere. 

        The volume buttons and the thumbstick button are already debounced on the AVR to reduce the I2C talk to minimum. 
     */
    struct ButtonState {
        bool current = false ISR_THREAD; 
        bool reported = false ISR_THREAD DRIVER_THREAD;
        unsigned debounce = 0 ISR_THREAD DRIVER_THREAD;
        unsigned autorepeat = 0 ISR_THREAD DRIVER_THREAD;
        Button button;
        unsigned const evdevId;

        /** Value to be reported when the button is pressed. 0 means it's normal button, other numbers mean the button reports the specified value on an absolute axis (its evdevId). */
        int axisValue = 0;

        /** Creates new button, parametrized by the pin and its evdev id. 
         */
        ButtonState(Button button, unsigned evdevId): button{button}, evdevId{evdevId} { }
 
        /** Creates new button represented by an axis and value. 
         */
        ButtonState(Button button, unsigned evdevId, int axisValue): button{button}, evdevId{evdevId}, axisValue{axisValue} { }

    }; // RCKid::Button

    /** An analog axis.

        We use the analog axes for the thumbstick and accelerometer. The thumbstick is debounced on the AVR.   
     */
    struct AxisState {
        uint8_t current;
        unsigned const evdevId;

        AxisState(unsigned evdevId):evdevId{evdevId} { }
    }; // RCKid:Axis

    struct Tick {};
    struct SecondTick {};
    struct Irq { unsigned pin; };
    struct KeyPress{ int key; bool state; };
    struct EnableGamepad{ bool enable; };

    /** Event for the driver's main loop to react to. Events with specified numbers are changes on the specified pins.
    */
    using HWEvent = std::variant<
        Tick, 
        SecondTick,
        Irq, 
        KeyPress,
        EnableGamepad,
        msg::AvrReset, 
        msg::Info, 
        msg::StartAudioRecording, 
        msg::StopAudioRecording,
        msg::SetBrightness,
        msg::SetTime, 
        msg::SetAlarm,
        msg::RumblerOk, 
        msg::RumblerFail, 
        msg::Rumbler,
        msg::RGBOn,
        msg::RGBOff,
        msg::RGBColor,
        msg::PowerOn,
        msg::PowerDown
    >;

    static RCKid * & instance() {
        static RCKid * i;
        return i;
    }

    /** The UI loop, should be called by the window's loop function. Processes the events from the driver to the main thread and calls the specific event handlers in the window. */
    void loop() MAIN_THREAD;

    void processEvent(Event & e) MAIN_THREAD; 

    /** The HW loop, proceses events from the hw event queue. This method is executed in a separate thread, which isthe only thread that accesses the GPIOs and i2c/spi connections. 
     */
    void hwLoop() DRIVER_THREAD;

#if (defined ARCH_MOCK)
    void checkMockButtons() DRIVER_THREAD;    
#endif

    /** Queries the accelerometer status and updates the X and Y accel axes. 
     */
    void accelQueryStatus() DRIVER_THREAD;

    /** Gets the status byte from the AVR. 
     */
    comms::Status avrQueryStatus() DRIVER_THREAD;

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

    void processAvrExtendedInfo(comms::ExtendedInfo const & einfo) DRIVER_THREAD;

    /** Receives new batch of audio recording from the AVR together with the status information. 
     */
    void avrGetRecording();

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
        static_assert(std::is_base_of<msg::Message, T>::value, "only applicable for mesages");
        using namespace platform;
        i2c::transmit(AVR_I2C_ADDRESS, reinterpret_cast<uint8_t const *>(& cmd), sizeof(T), nullptr, 0);
    }

    static void isrAvrIrq() ISR_THREAD {
        RCKid * self = RCKid::instance();
        self->hwEvents_.send(Irq{PIN_AVR_IRQ});
    }

    static void isrNrfIrq() ISR_THREAD {
        RCKid * self = RCKid::instance();
        self->hwEvents_.send(Irq{PIN_NRF_IRQ});
    }

    static void isrHeadphones() {
        RCKid * self = RCKid::instance();
        self->events_.send(HeadphonesEvent{platform::gpio::read(PIN_HEADPHONES)});
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

    static void isrButtonJoy() {
        using namespace platform;
        RCKid * i = RCKid::instance();
        i->buttonChange(gpio::read(PIN_BTN_JOY), i->btnJoy_);
    }

    /** ISR for the hardware buttons and the headphones. 
     
        Together with the driver thread's ticks, the isr is responsibel for debouncing. Called by the ISR or driver thread depending on the button. 
     */
    void buttonChange(int level, ButtonState & btn) ISR_THREAD DRIVER_THREAD {
        // always set the state
        btn.current = ! level;
        // if debounce is 0, take action and set the debounce timer, otherwise do nothing
        if (btn.debounce == 0) {
            btn.debounce = BTN_DEBOUNCE_DURATION;
            buttonAction(btn);
        }
    }

    void buttonAction(ButtonState & btn) ISR_THREAD DRIVER_THREAD;

    void buttonTick(ButtonState & btn) DRIVER_THREAD {
        if (btn.debounce > 0 && --(btn.debounce) == 0)
            if (btn.reported != btn.current)
                buttonAction(btn);
        if (btn.reported && btn.autorepeat > 0 && --(btn.autorepeat) == 0)
            buttonAction(btn);
    }

    bool axisChange(uint8_t value, AxisState & axis) DRIVER_THREAD {
        if (axis.current != value) {
            axis.current = value;
            if (activeDevice_ != nullptr) {
                libevdev_uinput_write_event(activeDevice_, EV_ABS, axis.evdevId, value);
                libevdev_uinput_write_event(activeDevice_, EV_SYN, SYN_REPORT, 0);
            }
            return true;
        } else {
            return false;
        }
    }

    Window * window_;

    /** Hardware events sent to the driver's main loop. 
     */
    EventQueue<HWEvent> hwEvents_;

    /** EVents sent from the  ISR and comm threads to the main thread. 
     */
    EventQueue<Event> events_;

    /** RCKid's state, available from the main thead. 
     */
    struct Status {
        bool changed;
        comms::Mode mode;
        bool usb;
        bool charging;
        uint16_t vBatt = 420;
        uint16_t vcc = 430;
        int16_t avrTemp;
        int16_t accelTemp;
        bool headphones;
        unsigned volume = 0;
        bool wifi = true;
        bool wifiHotspot = true;
        std::string ssid = "Internet 10";
        bool nrf = true;
        uint8_t brightness;
        // determines if we are recording in the main thread
        bool recording = false;
    } status_;

    std::function<void(RecordingEvent &)> recordingCallback_ DRIVER_THREAD;
    bool recording_ = false DRIVER_THREAD;
    uint8_t recordingLastBatch_ = 0xff DRIVER_THREAD;


    /** The button state objects, managed by the ISR thread 
     */
    ButtonState btnVolDown_{Button::VolumeDown, KEY_RESERVED};
    ButtonState btnVolUp_{Button::VolumeUp, KEY_RESERVED};
    ButtonState btnJoy_{Button::Joy, BTN_THUMBL};
    ButtonState btnA_{Button::A, BTN_EAST};
    ButtonState btnB_{Button::B, BTN_SOUTH}; 
    ButtonState btnX_{Button::X, BTN_NORTH};
    ButtonState btnY_{Button::Y, BTN_WEST};
    ButtonState btnL_{Button::L, BTN_TL};
    ButtonState btnR_{Button::R, BTN_TR};
    ButtonState btnSelect_{Button::Select, BTN_SELECT};
    ButtonState btnStart_{Button::Start, BTN_START};
    ButtonState btnHome_{Button::Home, BTN_MODE};
    ButtonState btnDpadUp_{Button::Up, ABS_HAT0Y, -1};
    ButtonState btnDpadDown_{Button::Down, ABS_HAT0Y, 1};
    ButtonState btnDpadLeft_{Button::Left, ABS_HAT0X, -1};
    ButtonState btnDpadRight_{Button::Right, ABS_HAT0X, 1};

    /** Axes. 
     */
    AxisState thumbX_{ABS_X};
    AxisState thumbY_{ABS_Y};
    AxisState accelX_{ABS_RX};
    AxisState accelY_{ABS_RY};

    /** Last known state so that we can determine when to send an update
     */
    ModeEvent mode_;
    ChargingEvent charging_;
    VoltageEvent voltage_;
    TempEvent temp_;
    BrightnessEvent brightness_;

    platform::NRF24L01 radio_{PIN_NRF_CS, PIN_NRF_RXTX};
    platform::MPU6050 accel_;

    /** Libevdev gamepad. 
     */

    struct libevdev * gamepadDev_{nullptr};
    struct libevdev_uinput * gamepad_{nullptr};
    struct libevdev_uinput * activeDevice_{nullptr};

}; // RCKid
