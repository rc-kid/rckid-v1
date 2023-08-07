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

#define PIN_BTN_DPAD_UP 255
#define PIN_BTN_DPAD_DOWN 255
#define PIN_BTN_DPAD_LEFT 255
#define PIN_BTN_DPAD_RIGHT 255

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

    enum class NRFState {
        Error, // Not present
        PowerDown, 
        Standby, 
        Rx,
        Tx,
    };

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

    /** Returns the next UI event waiting to be processed, if any. 
     */
    std::optional<Event> nextEvent();

    /** Turns RCKid off. 
     
        Tells the AVR to enter the power down mode. AVR does this and then waits for the RPI_POWEROFF signal, while when we detect the transition to powerOff state actually happening, we do rpi shutdown in the main loop.  
    */
    void powerOff() {
        TraceLog(LOG_INFO, "Power down initiated from RPi");
        driverEvents_.send(msg::PowerDown{});
    }

    uint8_t brightness() { std::lock_guard<std::mutex> g{mState_}; return state_.einfo.brightness(); }

    void setBrightness(uint8_t brightness) { driverEvents_.send(msg::SetBrightness{brightness}); }

    /** \name Input controls
     */
    //@{
    bool gamepadActive() const { std::lock_guard<std::mutex> g{mState_}; return gamepadActive_; }

    void setGamepadActive(bool value = true) { std::lock_guard<std::mutex> g{mState_}; gamepadActive_ = value; }

    void keyPress(int key, bool state) { driverEvents_.send(KeyPress{key, state}); }

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

    /** Returns the current audio volume. 
     */
    int volume() const { return volume_; }

    /** Sets the current audio volume
     */
    void setVolume(int value) {
        if (value < 0)
            value = 0;
        if (value > AUDIO_MAX_VOLUME)
            value = AUDIO_MAX_VOLUME;
        volume_ = value;
        system(STR("amixer sset -q Headphone -M " << volume_ << "%").c_str());
    }



    void startAudioRecording() {
        TraceLog(LOG_DEBUG, "Recording start");
        driverEvents_.send(msg::StartAudioRecording{});
#if (defined ARCH_MOCK)
        mockRecBatch_ = 0;
        mockRecording_ = true;
#endif
    }

    void stopAudioRecording() {
        TraceLog(LOG_DEBUG, "Recording stopped");
        driverEvents_.send(msg::StopAudioRecording{}); 
#if (defined ARCH_MOCK)
        mockRecording_ = false;
#endif
    }
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
        return nrfState_;
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
    struct SecondTick {};
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
        SecondTick,
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
    void processAvrExtendedState(comms::ExtendedState & state);

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

    void buttonAction(ButtonState & btn) {
        bool gamepadActive;
        {
            std::lock_guard<std::mutex> g{mState_};
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
    uint8_t volume_;

    struct libevdev * gamepadDev_{nullptr};
    struct libevdev_uinput * gamepad_{nullptr};
    bool gamepadActive_{false}; // protected by mState_

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

#ifdef HAHA
/** RCKid Driver

    Communicates with the attached hardware - AVR, NRF, accelerometer and photoresistor.
 */
class RCKid {
public:

    static inline std::filesystem::path const MUSIC_ARTWORK_DIR{"/rckid/images/music artwork"};

    static constexpr unsigned int RETROARCH_HOTKEY_ENABLE = BTN_THUMBR;
    static constexpr unsigned int RETROARCH_HOTKEY_PAUSE = BTN_SOUTH; // B
    static constexpr unsigned int RETROARCH_HOTKEY_SAVE_STATE = BTN_NORTH; // A
    static constexpr unsigned int RETROARCH_HOTKEY_LOAD_STATE = BTN_WEST; // X
    static constexpr unsigned int RETROATCH_HOTKEY_SCREENSHOT = BTN_EAST; // Y
    
    //static constexpr char const * LIBEVDEV_GAMEPAD_NAME = "rckid-gamepad";
    static constexpr char const * LIBEVDEV_KEYBOARD_NAME = "rckid-keyboard";

    static constexpr uint8_t BTN_DEBOUNCE_DURATION = 2;

    static constexpr uint8_t BTN_AUTOREPEAT_DURATION = 0; // 20; 

    /** Initializes the RCKid driver. 

        The initializer starts the hw loop and initializes the libevdev gamepad layer. 
     */
    static RCKid & create();

    ~RCKid() {
        shouldTerminate_.store(true);
        hwEvents_.send(Terminate{});
        tHwLoop_.join();
        tTicks_.join();
        tSeconds_.join();
        libevdev_uinput_destroy(gamepad_);
        libevdev_free(gamepadDev_);
    }

    /** Returns next RCKid event, or none if there are no events to be processed. This has to be called frequently, otherwise the RCKid's synchronization between the HW and UI threads will be broken. 
     */
    std::optional<Event> nextEvent() UI_THREAD; 

    /** Enables or disables automatic sending of button & analog events to the virtual gamepad. 
     */
    void enableGamepad(bool enable) {
        hwEvents_.send(EnableGamepad{enable});
    }

    void keyPress(int key, bool state) {
        hwEvents_.send(KeyPress{key, state});
    }

    /** Turns RCKid off. 
     
        Tells the AVR to enter the power down mode. AVR does this and then waits for the RPI_POWEROFF signal, while when we detect the transition to powerOff state actually happening, we do rpi shutdown in the main loop.  
    */
    void powerOff() {
        TraceLog(LOG_INFO, "Power down initiated from RPi");
        hwEvents_.send(msg::PowerDown{});
    }

    uint32_t avrUptime() const {
        return status_.avrUptime;
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

    void startRecording() {
        TraceLog(LOG_DEBUG, "Recording start");
        status_.recording = true;
        hwEvents_.send(msg::StartAudioRecording{});
#if (defined ARCH_MOCK)
        mockRecBatch_ = 0;
        mockRecording_ = true;
#endif
    }

    void stopRecording() { 
        TraceLog(LOG_DEBUG, "Recording stopped");
        hwEvents_.send(msg::StopAudioRecording{}); 
        status_.recording = false; 
#if (defined ARCH_MOCK)
        mockRecording_ = false;
#endif
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

    bool nrfInitialize(char const * rxAddr, char const * txAddr, uint8_t channel = 86) {
        if (nrfState_ == NRFState::Error)
            return false;
        hwEvents_.send(NRFInitialize{rxAddr, txAddr, channel});
        nrfState_ = NRFState::Standby;
        // TODO ensbure that the initialize does indeed take the radio to standby mode
        return true;
    }

    NRFState nrfState() const {
        return nrfState_;
    }

    bool nrfStandby() {
        if (nrfState_ == NRFState::Error || nrfState_ == NRFState::Transmitting)
            return false;
        hwEvents_.send(NRFStateChange{NRFState::Standby});
        nrfState_ = NRFState::Standby;
        return true;
    }

    bool nrfPowerDown() {
        if (nrfState_ == NRFState::Error || nrfState_ == NRFState::Transmitting)
            return false;
        hwEvents_.send(NRFStateChange{NRFState::PowerDown});
        nrfState_ = NRFState::PowerDown;
        return true;
    }

    bool nrfEnableReceiver() {
        if (nrfState_ == NRFState::Error || nrfState_ == NRFState::Transmitting)
            return false;
        hwEvents_.send(NRFStateChange{NRFState::Receiver});
        nrfState_ = NRFState::Receiver;
        return true;
    }

    bool nrfTransmit(uint8_t const * packet, uint8_t length = 32) {
        if (nrfState_ == NRFState::Error)
            return false;
        hwEvents_.send(NRFTransmit{packet, length, false});
        nrfState_ = NRFState::Transmitting;
        return true;
    }

    bool nrfTransmitWithImmediateReturn(uint8_t const * packet, uint8_t length = 32) {
        if (nrfState_ == NRFState::Error)
            return false;
        hwEvents_.send(NRFTransmit{packet, length, true});
        nrfState_ = NRFState::Transmitting;
        return true;
    }

    size_t nrfTxQueueSize() const { return nrfTxQueueSize_; }

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

private:

    friend class Window;

    friend RCKid & rckid() {
        return * RCKid::singleton_;
    }

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

    struct Terminate{};
    struct Tick {};
    struct SecondTick {};
    struct Irq { unsigned pin; };
    struct KeyPress{ int key; bool state; };
    struct EnableGamepad{ bool enable; };
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
    struct NRFStateChange{ NRFState state; };
    struct NRFTransmit{
        uint8_t packet[32]; 
        bool immediateReturn;

        NRFTransmit(uint8_t const * packet, uint8_t length, bool immediateReturn):
            immediateReturn{immediateReturn} {
            memcpy(this->packet, packet, length);
        }
    };



    /** Event for the driver's main loop to react to. Events with specified numbers are changes on the specified pins.
    */
    using HWEvent = std::variant<
        Terminate, 
        Tick, 
        SecondTick,
        Irq, 
        KeyPress,
        EnableGamepad,
        NRFInitialize, 
        NRFStateChange,
        NRFTransmit,
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

    /** Private constructor for the singleton object. 
     */
    RCKid(); 

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


    void nrfReceivePackets();

    void nrfTxDone();


    /** Initializes the ISRs on the rpi pins so that the driver can respond properly.
     */
    void initializeISRs() UI_THREAD;

    /** Initializes the libevdev gamepad device for other applications. 
     */
    void initializeLibevdevGamepad() UI_THREAD;

    void initializeAvr() UI_THREAD;

    void initializeAccel() UI_THREAD;

    void initializeNrf() UI_THREAD;

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
        if (btn.reported && (btn.autorepeat > 0) && (--(btn.autorepeat) == 0))
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
        uint8_t brightness;
        // determines if we are recording in the main thread
        bool recording = false;
        uint32_t avrUptime = 0; 
    } status_;


    // status of the NRF chip.
    NRFState nrfState_ UI_THREAD;
    size_t nrfTxQueueSize_ UI_THREAD;

    struct {
        uint32_t avrUptime = 0;

        bool recording = false;

        NRFState nrfState;
        bool nrfReceiveAfterTransmit;
        std::deque<NRFTransmit> nrfTxQueue;
    } driverStatus_ DRIVER_THREAD;


#if (defined ARCH_MOCK)
    volatile bool mockRecording_ = false;
    volatile uint8_t mockRecBatch_ = 0;
#endif

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

    platform::NRF24L01 nrf_{PIN_NRF_CS, PIN_NRF_RXTX};
    platform::MPU6050 accel_;

    /** Libevdev gamepad. 
     */

    struct libevdev * gamepadDev_{nullptr};
    struct libevdev_uinput * gamepad_{nullptr};
    struct libevdev_uinput * activeDevice_{nullptr};

    std::thread tHwLoop_;
    std::thread tTicks_;
    std::thread tSeconds_;
    std::atomic<bool> shouldTerminate_{false};

    static inline std::unique_ptr<RCKid> singleton_;

}; // RCKid


#endif