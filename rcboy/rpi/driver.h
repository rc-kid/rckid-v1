#pragma once

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <thread>

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-uinput.h"

#include <QObject>

#include "platform/platform.h"
#include "peripherals/nrf24l01.h"
#include "peripherals/mpu6050.h"

#include "common/comms.h"


/** RCBoy driver
 
    Controls the hardware such as the AVR, accelerometer, radio, neopixels and vibration motor.
 */
class Driver : public QObject {
    Q_OBJECT
public:

    /** Initializes the driver and returns its instance. 
     
        As part of the initialization, starts the driver's thread. 
     */
    static Driver * initialize();

    /** Returns the instance of previously installed driver. 
     
        Must be called *after* initialize(), otherwise returns nullptr. 
     */
    static Driver * instance() { return singleton_; }
    
    /** Starts the power off sequence. 
     */
    void poweroff() {
        emitEvent(Event::PowerOff);
    }

    /** Display Brightness
     */
    uint8_t brightness() const { return brightness_; }

    void setBrightness(uint8_t value) {
        {
            std::lock_guard g_{mState_};
            if (brightness_ != value)
                brightness_ = value;
        }
        emitEvent(Event::SetBrightness);
    }

    bool headphones() const { return headphones_; }

    uint16_t batteryVoltage() const { return batteryVoltage_; }
    uint8_t batteryVoltagePct() const { return batteryVoltage_ - 330; }

signals:

    /** \name Gamepad signals. 
     */
    //@{
    void buttonStart(bool state);
    void buttonSelect(bool state);
    void buttonA(bool state);
    void buttonB(bool state);
    void buttonX(bool state);
    void buttonY(bool state);
    void buttonLeft(bool state);
    void buttonRight(bool state);
    void buttonThumb(bool state);
    void buttonVolumeLeft(bool state);
    void buttonVolumeRight(bool state);
    void thumbstick(uint8_t x, uint8_t y);
    void accel(uint8_t x, uint8_t y); 
    //@}

    void headphonesChanged(bool state);
    void chargingChanged(bool state);
    void lowBatteryChanged();
    void batteryVoltageChanged(uint16_t value);
    void tempAvrChanged(uint16_t value); 
    void tempAccelChanged(uint16_t value); 

private:


    /** \name Pinout
     */
    //@{
    static constexpr unsigned PIN_AVR_IRQ = 4;
    static constexpr unsigned PIN_HEADPHONES = 17;

    static constexpr unsigned PIN_BTN_A = 24;
    static constexpr unsigned PIN_BTN_B = 22;
    static constexpr unsigned PIN_BTN_X = 0;
    static constexpr unsigned PIN_BTN_Y = 26;
    static constexpr unsigned PIN_BTN_L = 18;
    static constexpr unsigned PIN_BTN_R = 27;
    static constexpr unsigned PIN_BTN_SELECT = 23;
    static constexpr unsigned PIN_BTN_START = 1;

    static constexpr unsigned PIN_NRF_CS = 16;
    static constexpr unsigned PIN_NRF_RXTX = 5;
    static constexpr unsigned PIN_NRF_IRQ = 6;
    //@}

    /** Events the main loop responds to. These come either from IRQ handlers, or Qi or other threads requesting AVR To perform tasks. 
     */
    enum class Event {
        AvrIrq, 
        RadioIrq,
        SetBrightness,


        PowerOff,
    }; // Driver::Event
    
    struct Button {
        bool state;
        unsigned evdevId;

        Button(unsigned evdevId): state{false}, evdevId{evdevId}  {}

        bool update(bool newState);

    }; // Driver::Button

    struct Axis {
        uint8_t state;
        unsigned evdevId;

        Axis(unsigned evdevId): state{127}, evdevId{evdevId} {}

        bool update(uint8_t newState) {
            if (state != newState) {
                state = newState;
                return true;
            } else {
                return false;
            }
        }

    }; // Driver::Axis

    /** The main loop. 
     
        Reacts to any events from the interrupts and periodically talks to the AVR and accelerometer to update the gamepad and stuff. 
     */
    void loop();

    void emitEvent(Event e) {
        std::lock_guard<std::mutex> g{m_};
        events_.push_back(e);
        cv_.notify_all();
    }

    /** Queries avr for the current status. 
     
        
     */
    void queryAvrStatus();

    /** Queries the full AVR information. 
     */
    void queryAvrFull();



    void updateStatus(comms::Status status);

    /** Queries the accelerometer for the current state, 
     */
    void queryAccel();

    /** Converts the accelerometer values to 1G range so that it can be used for positioning. 
     */
    uint8_t accelTo1GUnsigned(int16_t v); 

    void queryRadio();

    /** \name Interrupt handlers. 
     
        The interrupt handlers should be fast and not do anything complex, i.e. no talking to QT or any peripherals in them. Instead, an isr simply adds a particular event to the event queue where it gets resolved by the driver's main thread. 
        
        This does add some latency to the event processing, but should relly be negligible in the grand scheme of things.  
     */
    //@{


    static void isrAvrIrq(int gpio, int level, uint32_t tick, Driver * self) {
        self->emitEvent(Event::AvrIrq);
    }

    static void isrRadio(int gpio, int level, uint32_t tick, Driver * self) {
        self->emitEvent(Event::RadioIrq);
    }

    static void isrButtonChange(int gpio, int level, uint32_t tick, Button * btn);

    static void isrHeadphonesChange(int gpio, int level, uint32_t tick, Button * btn);

    //@}


    void initializeLibevdevDevice();
    void initializePins();

    static inline  Driver * singleton_ = nullptr;


    std::mutex m_;
    std::condition_variable cv_;
    std::deque<Event> events_;




    SpinLock mState_;

    /** \name Gamepad
     */
    //@{
    Button a_{BTN_A};
    Button b_{BTN_B};
    Button x_{BTN_X};
    Button y_{BTN_Y};
    Button l_{BTN_LEFT};
    Button r_{BTN_RIGHT};
    Button select_{BTN_SELECT};
    Button start_{BTN_START};
    Button volumeLeft_{KEY_VOLUMEDOWN};
    Button volumeRight_{KEY_VOLUMEUP};
    Button thumbBtn_{BTN_JOYSTICK};
    Axis thumbX_{ABS_RX};
    Axis thumbY_{ABS_RY};
    Axis accelX_{ABS_X};
    Axis accelY_{ABS_Y};
    Axis accelZ_{ABS_Z};
    
    /** Handle for the uinput device of the gamepad.
     */
    struct libevdev_uinput *uidev_{nullptr};
    //@}

    bool headphones_;
    bool charging_;
    bool lowBattery_;

    /** Measured voltage battery to determine the capacity. 
     */
    uint16_t batteryVoltage_;

    /** Temperature measured by the AVR. 
     */
    uint16_t tempAvr_;
    /** Temperature measured by the accelerometer chip. 
     */
    uint16_t tempAccel_; 

    /** Display brightness. 
     */
    uint8_t brightness_;


    NRF24L01 radio_{PIN_NRF_CS, PIN_NRF_RXTX};
    MPU6050 accel_;

}; // Controller