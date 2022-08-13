#pragma once

#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-uinput.h"

#include "platform/platform.h"
#include "peripherals/mpu6050.h"


/** The gamepad driver class.
 
    The driver is also responsible for communication with the AVR and the accelerometer. It runs in a dedicated thread so as not to interfere with the gui. 
 */
class Gamepad {
public:

    static constexpr char const * DEVICE_NAME = "rcboy-gamepad";

    /** Initializes the gamepad and comms. 
     */
    static void initialize() {
        initializeLibevdevDevice();
        initializePins();
        initializeI2C();
        std::thread t{Gamepad::loop};
        t.detach();
    }

    /** The main loop. 
     
        The buttons connected to rpi are handled immediately in the isr routine, while the main loop takes care of the communication with the AVR and the accelerometer. 
     */
    static void loop(); 
    

private:

    /** \name Pinout description
     
        Pins for the communication and gamepad itself.  
     */
    //@{
    static constexpr unsigned GPIO_AVR_IRQ = 4;

    static constexpr unsigned GPIO_A = 24;
    static constexpr unsigned GPIO_B = 22;
    static constexpr unsigned GPIO_X = 0;
    static constexpr unsigned GPIO_Y = 26;
    static constexpr unsigned GPIO_L = 18;
    static constexpr unsigned GPIO_R = 27;
    static constexpr unsigned GPIO_SELECT = 23;
    static constexpr unsigned GPIO_START = 1;
    //@}

    enum class Event {
        AVR_IRQ,
    }; // Gamepad::Event

    class ButtonInfo {
    public:
        bool state;
        unsigned evdevId;

        ButtonInfo(unsigned evdevId): state{false}, evdevId{evdevId} {}

    }; // Gamepad::ButtonInfo

    struct AxisInfo {
        uint8_t state;
        unsigned evdevId;
    
        AxisInfo(unsigned evdevId): state{127}, evdevId{evdevId} {}

    }; // Gamepad::AxisInfo

    /** Queries the avr for its state. 
     */    
    static void queryAvr();

    /** Queries the accelerometer for its state once every 5ms (200Hz). 
     */
    static void queryAccelerometer();

    /** Creates and registers the libevdev gamepad device. 
     */
    static void initializeLibevdevDevice();

    /** Initializes the rpi pins and their interrupts. 
     */
    static void initializePins();

    /** Initializes the I2C communication between rpi, avr and accelerometer. 
     */
    static void initializeI2C();

    static void isrButtonChange(int gpio, int level, uint32_t tick, ButtonInfo * btn) {
        bool state = ! level;
        if (btn->state == state)
            return;
        // TODO debounce
        btn->state = state;
        if (btn->evdevId != KEY_RESERVED) {
            libevdev_uinput_write_event(Gamepad::uidev_, EV_KEY, btn->evdevId, state ? 1 : 0);
            libevdev_uinput_write_event(Gamepad::uidev_, EV_SYN, SYN_REPORT, 0);
        }
    }

    /** Don't check the avr state in the isr, notify the main thread instead.  
     */
    static void isrAvrIrq(int gpio, int level, uint32_t tick, Gamepad * self) {
        {
            std::lock_guard g{m_};
            q_.push_back(Event::AVR_IRQ);
        }
        cv_.notify_one();
    }

    static inline std::mutex m_;
    static inline std::condition_variable cv_;
    static inline std::deque<Event> q_;

    /** \name Gamepad state
     */
    //@{

    inline static ButtonInfo a_{BTN_A};
    inline static ButtonInfo b_{BTN_B};
    inline static ButtonInfo x_{BTN_X};
    inline static ButtonInfo y_{BTN_Y};
    inline static ButtonInfo l_{BTN_LEFT};
    inline static ButtonInfo r_{BTN_RIGHT};
    inline static ButtonInfo select_{BTN_SELECT};
    inline static ButtonInfo start_{BTN_START};
    inline static ButtonInfo volumeLeft_{KEY_VOLUMEDOWN};
    inline static ButtonInfo volumeRight_{KEY_VOLUMEUP};

    inline static AxisInfo thumbX_{ABS_RX};
    inline static AxisInfo thumbY_{ABS_RY};
    inline static AxisInfo accelX_{ABS_X};
    inline static AxisInfo accelY_{ABS_Y};
    inline static AxisInfo accelZ_{ABS_Z};
    //@}

    /** Handle for the uinput device of the gamepad.
     */
    inline static struct libevdev_uinput *uidev_{nullptr};

    /** Accelerometer (connected via I2C)
     */
    inline static MPU6050 accel_;

}; // Gamepad