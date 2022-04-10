
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <deque>


#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/uinput.h>
//#include <linux/i2c-dev.h>
//#include <i2c/smbus.h>

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-uinput.h"

#include "platform/platform.h"
#include "peripherals/mpu6050.h"
#include "peripherals/nrf24l01.h"

/** Gamepad Driver.
 
    The driver creates a new gamepad device via uinput and libevdev and publishes to it the gamepad events as read from RPi GPIOs, accelerometer and the AVR, so that the gamepad looks like a real gamepad connected to the RPi. 
 */
class Gamepad {
public:

    /** Pins at which the buttons are connected directly to RPi. 
     */
    static constexpr unsigned GPIO_A = 17;
    static constexpr unsigned GPIO_B = 18;
    static constexpr unsigned GPIO_X = 27;
    static constexpr unsigned GPIO_Y = 22;
    static constexpr unsigned GPIO_L = 23;
    static constexpr unsigned GPIO_R = 24;

    static constexpr unsigned GPIO_AVR_IRQ = 4;

    enum class Button : unsigned {
        A = BTN_A,
        B = BTN_B, 
        X = BTN_X, 
        Y = BTN_Y,
        Left = BTN_LEFT,
        Right = BTN_RIGHT,
        Select = BTN_SELECT,
        Start = BTN_START,
    };

    enum class Axis : unsigned {
        ThumbX = ABS_RX,
        ThumbY = ABS_RY,
        AccelX = ABS_X,
        AccelY = ABS_Y,
        AccelZ = ABS_Z,
    };

    Gamepad() {
        initializeDevice();
        initializeGPIO();
        initializeI2C();
    }

    ~Gamepad() {
        libevdev_uinput_destroy(uidev_);
        //gpioTerminate();
    }

    /** The main loop of the gamepad driver in which the buttons, AVR and accelerometer are monitored and updates are sent to the uinput device. 
     
        As the driver is the sole communication point between the AVR and RPi it also collects all other information available and makes sure to perform the periodic heartbeat updates so that AVR knows we are alive.

        The querying happens as such
     */
    void loop();

private:

    /** An event from the ISR handlers to that main thread that is responsible for the communication and input device emulation. 
     */
    enum class Event {
        ButtonAChange,
        ButtonBChange,
        ButtonXChange,
        ButtonYChange,
        ButtonLeftChange,
        ButtonRightChange,
        AvrIrq,
    };

    void queryAccelerometer();

    void queryAVR();

    static void isrAvrIrq(int gpio, int level, uint32_t tick, Gamepad * gamepad);


    void initializeDevice();
    void initializeGPIO();
    void initializeI2C();

    void buttonChange(Button btn, bool pressed) {
        libevdev_uinput_write_event(uidev_, EV_KEY, static_cast<unsigned>(btn), pressed ? 1 : 0);
        libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
    }

    void axisChange(Axis axis, int value) {
        libevdev_uinput_write_event(uidev_, EV_ABS, static_cast<unsigned>(axis), value);
        libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
    }

    void addEvent(Event event) {
        {
            std::lock_guard g{m_};
            q_.push_back(event);
        }
        cv_.notify_one();
    }


    /** ISRs for the buttons connected directly to RPI GPIOs. 
     */
    static void isrBtnA(int gpio, int level, uint32_t tick, Gamepad * gamepad);
    static void isrBtnB(int gpio, int level, uint32_t tick, Gamepad * gamepad);
    static void isrBtnX(int gpio, int level, uint32_t tick, Gamepad * gamepad);
    static void isrBtnY(int gpio, int level, uint32_t tick, Gamepad * gamepad);
    static void isrBtnL(int gpio, int level, uint32_t tick, Gamepad * gamepad);
    static void isrBtnR(int gpio, int level, uint32_t tick, Gamepad * gamepad);

    std::mutex m_;
    std::condition_variable cv_;
    std::deque<Event> q_;

    /** Handle for the uinput device of the gamepad.
     */
    struct libevdev_uinput *uidev_;

    /** Current button levels. Note that these are reversed due to the internal pull-ups.
     */
    bool a_ = true;
    bool b_ = true;
    bool x_ = true;
    bool y_ = true;
    bool l_ = true;
    bool r_ = true;

    bool sel_ = true;
    bool start_ = true;

    /** Current axis levels. 
     */
    uint8_t accelX_ = 128;
    uint8_t accelY_ = 128;
    uint8_t accelZ_ = 128;
    uint8_t thumbX_ = 128;
    uint8_t thumbY_ = 128;

    /** The I2C accelerometer & gyroscope. 
     */
    MPU6050 accel_;



}; // Gamepad
