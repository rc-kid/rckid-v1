#pragma once

#include <queue>
#include <mutex>
#include <variant>
#include <functional>
#include <memory>
#include <atomic>
#include <filesystem>
#include <utils/locks.h>

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-uinput.h"

//#if (!defined ARCH_RPI)
#undef KEY_APOSTROPHE
#undef KEY_COMMA
#undef KEY_MINUS
#undef KEY_SLASH
#undef KEY_SEMICOLON
#undef KEY_EQUAL
#undef KEY_A
#undef KEY_B
#undef KEY_C
#undef KEY_D
#undef KEY_E
#undef KEY_F
#undef KEY_G
#undef KEY_H
#undef KEY_I
#undef KEY_J
#undef KEY_K
#undef KEY_L
#undef KEY_M
#undef KEY_N
#undef KEY_O
#undef KEY_P
#undef KEY_Q
#undef KEY_R
#undef KEY_S
#undef KEY_T
#undef KEY_U
#undef KEY_V
#undef KEY_W
#undef KEY_X
#undef KEY_Y
#undef KEY_Z
#undef KEY_BACKSLASH
#undef KEY_GRAVE
#undef KEY_SPACE
#undef KEY_ENTER
#undef KEY_TAB
#undef KEY_BACKSPACE
#undef KEY_INSERT
#undef KEY_DELETE
#undef KEY_RIGHT
#undef KEY_LEFT
#undef KEY_DOWN
#undef KEY_UP
#undef KEY_HOME
#undef KEY_END
#undef KEY_PAUSE
#undef KEY_F1
#undef KEY_F2
#undef KEY_F3
#undef KEY_F4
#undef KEY_F5
#undef KEY_F6
#undef KEY_F7
#undef KEY_F8
#undef KEY_F9
#undef KEY_F10
#undef KEY_F11
#undef KEY_F12
#undef KEY_BACK
#undef KEY_MENU
//#endif

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
             DISPLAY_DC -- 22*    *23 -- BTN_Y
                           3V3    *24 -- RPI_POWEROFF
    DISPLAY MOSI (SPI0) -- 10     GND
    DISPLAY MISO (SPI0) -- 9      *25 -- AVR_IRQ
    DISPLAY SCLK (SPI0) -- 11       8 -- DISPLAY CE (SPI0 CE0)
                           GND      7 -- DPAD_LEFT
            DPAD_BOTTOM -- 0        1 -- DPAD_UP
             DPAD_RIGHT -- 5*     GND
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
#define PIN_BTN_DPAD_LEFT 7
#define PIN_BTN_DPAD_DOWN 0
#define PIN_BTN_DPAD_RIGHT 5
#define PIN_BTN_DPAD_UP 1
#define PIN_BTN_JOY 26

#define UI_THREAD
#define DRIVER_THREAD
#define ISR_THREAD

class Window; 
class RCKid;

RCKid & rckid(); 

class RCKid {
public:

    static constexpr unsigned int RETROARCH_HOTKEY_ENABLE = BTN_THUMBR;
    static constexpr unsigned int RETROARCH_HOTKEY_PAUSE = BTN_SOUTH; // B
    static constexpr unsigned int RETROARCH_HOTKEY_SAVE_STATE = BTN_NORTH; // A
    static constexpr unsigned int RETROARCH_HOTKEY_LOAD_STATE = BTN_WEST; // X
    static constexpr unsigned int RETROATCH_HOTKEY_SCREENSHOT = BTN_EAST; // Y

    static RCKid & create();

    /** Terminate the driver & event threads and release the gamepad resources. 
     */
    ~RCKid() {
        shouldTerminate_.store(true);
        driverEvents_.send(Terminate{});
        tHwLoop_.join();
        tTicks_.join();
        tSeconds_.join();
        libevdev_uinput_destroy(gamepad_);
        libevdev_free(gamepadDev_);
    }

    /** Returns the status RCKid's status alone. 
     */
    comms::Status status() const { std::lock_guard<std::mutex> g{mState_}; return state_.status; }

    /** Returns the full RCKid's extended state. 
     */
    comms::ExtendedState extendedState() const { std::lock_guard<std::mutex> g{mState_}; return state_; }

    int16_t accelTemp() const { std::lock_guard<std::mutex> g{mState_}; return accelTemp_; }

    uint16_t hearts() const { return pState_.hearts; }

    void setHearts(uint16_t value) { pState_.hearts = value; }

    bool heartsCounterEnabled() const { return heartsCounterEnabled_ > 0; }

    void enableHeartsCounter(bool value = true) { 
        if (value) {
            ++heartsCounterEnabled_;
        } else {
            if (heartsCounterEnabled_ > 0)
                --heartsCounterEnabled_;
        }
    }

    /** Returns the next UI event waiting to be processed, if any. 
     */
    std::optional<Event> nextEvent();

    /** Turns RCKid off. 
     
        Tells the AVR to enter the power down mode. AVR does this and then waits for the RPI_POWEROFF signal, while when we detect the transition to powerOff state actually happening, we do rpi shutdown in the main loop.  
    */
    void powerOff();

    uint8_t brightness() { return pState_.brightness; }

    void setBrightness(uint8_t brightness) { 
        pState_.brightness = brightness;
        driverEvents_.send(msg::SetBrightness{brightness}); 
    }

    /** \name Input controls
     */
    //@{
    bool gamepadActive() const { std::lock_guard<std::mutex> g{mState_}; return gamepadActive_; }

    void setGamepadActive(bool value = true) { std::lock_guard<std::mutex> g{mState_}; gamepadActive_ = value; }

    void keyPress(int key, bool state) { driverEvents_.send(KeyPress{key, state}); }

    bool joyAsButtons() const { std::lock_guard<std::mutex> g{mState_}; return joyAsButtons_; }

    void setJoyAsButtons(bool value) { std::lock_guard<std::mutex> g{mState_}; joyAsButtons_ = value;}

    bool accelAsButtons() const { std::lock_guard<std::mutex> g{mState_}; return accelAsButtons_; }

    void setAccelAsButtons(bool value) { std::lock_guard<std::mutex> g{mState_}; accelAsButtons_ = value;}
    //@}


    /** \name RGB LED Control 
     */
    //@{

    void rgbOn() { driverEvents_.send(msg::RGBOn{}); }
    void rgbOff() { driverEvents_.send(msg::RGBOff{}); }
    void rgbColor(platform::Color color) { driverEvents_.send(msg::RGBColor{color}); }

    //@}
    /** \name Audio & Recording 
     */
    //@{
    bool headphones() { std::lock_guard<std::mutex> g{mState_}; return headphones_; }

    /** Returns the current audio volume. Returns volume as signed, but will always be 0..AUDIO_MAX_VOLUME
     */
    int volume() const { return pState_.volume; }

    /** Sets the current audio volume. Takes integer so that both too low and too high volumes outside the range can be clipped.  
     */
    void setVolume(int value) {
        if (value < 0)
            value = 0;
        if (value > AUDIO_MAX_VOLUME)
            value = AUDIO_MAX_VOLUME;
        pState_.volume = value;
        system(STR("amixer sset -q Headphone -M " << value << "%").c_str());
        // notify the main thread that there has been a state change (amongst other things refreshes the header)
        uiEvents_.send(StateChangeEvent{});
    }

    void startAudioRecording();

    void stopAudioRecording();
    //@}

    /** \name NRF Radio 
     
        

     */
    //@{
    void nrfInitialize(char const * rxAddr, char const * txAddr, uint8_t channel) {
        driverEvents_.send(NRFInitialize{rxAddr, txAddr, channel});
    }

    void nrfPowerDown() {
        driverEvents_.send(NRFState::PowerDown);
    }

    /** Enables the radio in receiver mode.
     */
    void nrfEnableReceiver() {
        driverEvents_.send(NRFState::Rx);
    }

    /** Enable the radio in transmitter mode, without transmitting any messages. 
     */
    void nrfEnableTransmitter() {
        driverEvents_.send(NRFState::Tx);
    }

    /** Transmits a message. If used in received mode, briefly stops the receiver, enters the transmitter mode and then transmits the message, returning to receiver mode afterwards. 
     */
    void nrfTransmit(uint8_t const * packet, uint8_t length = 32) {
        std::lock_guard<std::mutex> g{mRadio_};
        nrfTxQueue_.push_back(NRFPacket{packet, length});
        // if we are the first in the queue, we need to notify the driver thread to start sending
        if (nrfTxQueue_.size() == 1)
            driverEvents_.send(NRFTransmit{});
    }

    /** Transmits the given packet in immediate mode, to minimalize the time spent in tx mode not receiving. 
     */
    void nrfTransmitImmediate(uint8_t const * packet, uint8_t length = 32) {
        driverEvents_.send(NRFPacket{packet, length});
    }

    size_t nrfTxQueueSize() const {
        std::lock_guard<std::mutex> g{mRadio_};
        return nrfTxQueue_.size();
    }

    NRFState nrfState() const {
        std::lock_guard<std::mutex> g{mRadio_};
        return nrfTx_ ? NRFState::Tx : nrfState_;
    }

    //@}

private:

    struct ButtonState {
        Button const btn;
        unsigned const evdevId;
        size_t debounceTimer{0};
        bool actualState{false};
        bool reportedState{false}; // protected by mState_
        int reportValue;

        ButtonState(Button btn, unsigned evdevId, int reportValue = 0): btn{btn}, evdevId{evdevId}, reportValue{reportValue} {}

        bool update(bool state) {
            actualState = state;
            if (debounceTimer == 0) {
                if (reportedState != actualState) {
                    debounceTimer = BTN_DEBOUNCE_DURATION_TICKS;
                    return true;
                }
            }
            return false;
        }
    };

    struct AxisState {
        unsigned const evdevId;
        uint8_t actualValue{0};
        uint8_t reportedValue{0}; // protected by mState_

        AxisState(unsigned evdevId): evdevId{evdevId} {}

        bool update(uint8_t state) {
            actualValue = state;
            return actualValue != reportedValue;
        }
    }; 

    struct Terminate{};
    struct Tick {};
    struct AvrIrq {};
    struct NRFIrq {};
    struct NRFTransmit {};
    struct HeadphonesIrq { bool value; };
    struct ButtonIrq { ButtonState & btn; bool state; };
    struct KeyPress{ int key; bool state; };

    struct NRFInitialize{ 
        char rxAddr[5]; 
        char txAddr[5]; 
        uint8_t channel; 
        NRFInitialize(char const * rxAddr, char const * txAddr, uint8_t channel):
            channel{channel} {
            memcpy(this->rxAddr, rxAddr, 5);
            memcpy(this->txAddr, txAddr, 5);
        }
    };

    struct NRFPacket {
        uint8_t packet[32]; 
        NRFPacket(uint8_t const * packet, uint8_t length) {
            memcpy(this->packet, packet, length);
        }
    };

    using DriverEvent = std::variant<
        Terminate,
        Tick, 
        SecondTick, // ::SecondTick
        AvrIrq,
        NRFIrq, 
        HeadphonesIrq,
        ButtonIrq,
        KeyPress,
        NRFInitialize, 
        NRFTransmit,
        NRFPacket,
        NRFState,
        msg::AvrReset, 
        msg::Info, 
        msg::StartAudioRecording, 
        msg::StopAudioRecording,
        msg::SetBrightness,
        msg::SetTime, 
        msg::GetPersistentState,
        msg::SetPersistentState,
        msg::RumblerOk, 
        msg::RumblerFail, 
        msg::Rumbler,
        msg::RGBOn,
        msg::RGBOff,
        msg::RGBColor,
        msg::PowerOn,
        msg::PowerDown
    >;

    /** Private constructor for the singleton object. */
    RCKid();

    void processDriverEvent(DriverEvent e);

    void initializeAvr();
    /** Transmits the given command to the AVR. 
     */
    template<typename T>
    void sendAvrCommand(T const & cmd) DRIVER_THREAD {
        static_assert(std::is_base_of<msg::Message, T>::value, "only applicable for mesages");
        platform::i2c::transmit(AVR_I2C_ADDRESS, reinterpret_cast<uint8_t const *>(& cmd), sizeof(T), nullptr, 0);
    }
    comms::Status queryAvrStatus() {
        comms::Status status;
        platform::i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& status, sizeof(status));
        return status;
    }

    comms::State queryAvrState() {
        comms::State state;
        platform::i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
        return state;
    }

    comms::ExtendedState queryAvrExtendedState() {
        comms::ExtendedState state;
        platform::i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
        return state;
    }

    void getAvrRecording() {
        RecordingEvent r;
        platform::i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)(&r), sizeof(RecordingEvent));
        // do the normal status processing as we would in non-recording mode
        processAvrStatus(r.status);
        if (r.status.recording() && !r.status.batchIncomplete())
            uiEvents_.send(r);
    }

    void processAvrStatus(comms::Status status, bool alreadyLocked = false);
    void processAvrControls(comms::Controls controls, bool alreadyLocked = false);
    void processAvrExtendedState(comms::ExtendedState & state, bool alreadyLocked = false);

    void initializeAccel();
    void queryAccelStatus();

    void initializeNrf();
    
    void initializeISRs();
    static void isrAvrIrq() { RCKid::instance()->driverEvents_.send(AvrIrq{}); }
    static void isrNrfIrq() { RCKid::instance()->driverEvents_.send(NRFIrq{}); }
    static void isrHeadphones() { RCKid::instance()->driverEvents_.send(HeadphonesIrq{platform::gpio::read(PIN_HEADPHONES)}); }
    static void isrButtonA() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnA_, ! platform::gpio::read(PIN_BTN_A)}); }
    static void isrButtonB() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnB_, ! platform::gpio::read(PIN_BTN_B)}); }
    static void isrButtonX() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnX_, ! platform::gpio::read(PIN_BTN_X)}); }
    static void isrButtonY() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnY_, ! platform::gpio::read(PIN_BTN_Y)}); }
    static void isrButtonDpadUp() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnDpadUp_, ! platform::gpio::read(PIN_BTN_DPAD_UP)}); }
    static void isrButtonDpadDown() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnDpadDown_, ! platform::gpio::read(PIN_BTN_DPAD_DOWN)}); }
    static void isrButtonDpadLeft() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnDpadLeft_, ! platform::gpio::read(PIN_BTN_DPAD_LEFT)}); }
    static void isrButtonDpadRight() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnDpadRight_, ! platform::gpio::read(PIN_BTN_DPAD_RIGHT)}); }
    static void isrButtonJoy() { auto & i = RCKid::instance(); i->driverEvents_.send(ButtonIrq{i->btnJoy_, ! platform::gpio::read(PIN_BTN_JOY)}); }

    void initializeLibevdev();

    void buttonTick(ButtonState & btn) {
        if (btn.debounceTimer > 0 && --btn.debounceTimer == 0)
            if (btn.reportedState != btn.actualState)
                buttonAction(btn);
    }

    void buttonAction(ButtonState & btn, bool alreadyLocked = false) {
        bool gamepadActive;
        {
            utils::cond_lock_guard g{mState_, alreadyLocked};
            btn.reportedState = btn.actualState;
            gamepadActive = gamepadActive_;
        }
        // send the event to libevdev, if required
        if (gamepadActive && btn.evdevId != KEY_RESERVED && gamepad_ != nullptr) {
            if (btn.reportValue == 0)
                libevdev_uinput_write_event(gamepad_, EV_KEY, btn.evdevId, btn.actualState ? 1 : 0);
            else
                libevdev_uinput_write_event(gamepad_, EV_ABS, btn.evdevId, btn.actualState ? btn.reportValue : 0);
            libevdev_uinput_write_event(gamepad_, EV_SYN, SYN_REPORT, 0);
        }
        // send the event to the UI thread
        uiEvents_.send(ButtonEvent{btn.btn, btn.reportedState});
    }

    void axisAction(AxisState & axis, bool alreadyLocked = false) {
        bool gamepadActive;
        {
            utils::cond_lock_guard g{mState_, alreadyLocked};
            axis.reportedValue = axis.actualValue;
            gamepadActive = gamepadActive_;
        }
        // send the event to libevdev, if required
        if (gamepadActive && axis.evdevId != KEY_RESERVED && gamepad_ != nullptr) {
            libevdev_uinput_write_event(gamepad_, EV_ABS, axis.evdevId, axis.actualValue);
            libevdev_uinput_write_event(gamepad_, EV_SYN, SYN_REPORT, 0);
        }
        // note we can't send the ui event since the ui events are handled differently (thumb vs accel)
    }

    void readPersistentState();

    void writePersistentState();

    enum class AnalogButtonState {
        None, 
        Low, 
        High,
    };

    AnalogButtonState axisAsButton(uint8_t value, uint8_t d) {
        AnalogButtonState result{AnalogButtonState::None};
        if (value > 128 + d)
            result = AnalogButtonState::High;
        if (value < 128 - d)
            result = AnalogButtonState::Low;
        return result;
    }

    /** Hardware events sent to the driver's thread main loop from other threads. */
    EventQueue<DriverEvent> driverEvents_;

    /** Events sent from the  ISR and comm threads to the main thread. */
    EventQueue<Event> uiEvents_;

    /** Last known state of the AVR so that we can determine any changes and emit events. Protected by a mutex. */
    comms::ExtendedState state_;
    mutable std::mutex mState_;
    int16_t accelTemp_; 
    bool headphones_{false};



    /** Audio volume. Only accessible from the UI thread. */
    comms::PersistentState pState_;
    size_t heartsCounterEnabled_ = 0;

    struct libevdev * gamepadDev_{nullptr};
    struct libevdev_uinput * gamepad_{nullptr};
    bool gamepadActive_{false}; // protected by mState_
    bool joyAsButtons_{true}; 
    bool accelAsButtons_{false};

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
    ButtonState btnJoy_{Button::Joy, BTN_THUMBL};
    
    // virtual buttons used for the joystick
    ButtonState btnJoyUp_{Button::Up, ABS_HAT0Y, -1};
    ButtonState btnJoyDown_{Button::Down, ABS_HAT0Y, 1};
    ButtonState btnJoyLeft_{Button::Left, ABS_HAT0X, -1};
    ButtonState btnJoyRight_{Button::Right, ABS_HAT0X, 1};

    // virtual buttons for accelerometer 
    ButtonState btnAccelUp_{Button::Up, ABS_HAT0Y, -1};
    ButtonState btnAccelDown_{Button::Down, ABS_HAT0Y, 1};
    ButtonState btnAccelLeft_{Button::Left, ABS_HAT0X, -1};
    ButtonState btnAccelRight_{Button::Right, ABS_HAT0X, 1};

    AxisState joyX_{ABS_X};
    AxisState joyY_{ABS_Y};
    AxisState accelX_{ABS_RX};
    AxisState accelY_{ABS_RY};

    platform::MPU6050 accel_;

    platform::NRF24L01 nrf_{PIN_NRF_CS, PIN_NRF_RXTX};
    bool nrfTx_{false};
    NRFState nrfState_{NRFState::PowerDown};
    std::deque<NRFPacket> nrfTxQueue_;
    mutable std::mutex mRadio_;


    std::thread tHwLoop_;
    std::thread tTicks_;
    std::thread tSeconds_;
    std::atomic<bool> shouldTerminate_{false};

    static inline std::unique_ptr<RCKid> & instance() {
        static std::unique_ptr<RCKid> instance_;
        return instance_;
    };

    friend RCKid & rckid() {
        return * RCKid::instance();
    }

#if (defined ARCH_MOCK)
    volatile bool mockRecording_ = false;
    volatile uint8_t mockRecBatch_ = 0;

    void checkMockButtons();
#endif

}; // RCKid
