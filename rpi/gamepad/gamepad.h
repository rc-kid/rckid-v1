
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>


#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/uinput.h>
//#include <linux/i2c-dev.h>
//#include <i2c/smbus.h>

#include <pigpio.h>

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-uinput.h"


class Gamepad {
public:

    static constexpr unsigned GPIO_A = 17;
    static constexpr unsigned GPIO_B = 18;
    static constexpr unsigned GPIO_X = 27;
    static constexpr unsigned GPIO_Y = 22;

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
        gpioTerminate();
    }

    void loop();


private:


    void initializeDevice();
    void initializeGPIO();
    void initializeI2C();

    void buttonPress(Button btn) {
        libevdev_uinput_write_event(uidev_, EV_KEY, static_cast<unsigned>(btn), 1);
        libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
    }

    void buttonRelease(Button btn) {
        libevdev_uinput_write_event(uidev_, EV_KEY, static_cast<unsigned>(btn), 0);
        libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
    }

    void axisChange(Axis axis, int value) {
        libevdev_uinput_write_event(uidev_, EV_ABS, static_cast<unsigned>(axis), value);
        libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
    }

    static void isrBtnA(int gpio, int level, uint32_t tick, Gamepad * gamepad);
    static void isrBtnB(int gpio, int level, uint32_t tick, Gamepad * gamepad);
    static void isrBtnX(int gpio, int level, uint32_t tick, Gamepad * gamepad);
    static void isrBtnY(int gpio, int level, uint32_t tick, Gamepad * gamepad);

    struct libevdev_uinput *uidev_;

    /* Current button levels. Note that these are reversed due to the internal pull-ups.
     */
    bool a_ = true;
    bool b_ = true;
    bool x_ = true;
    bool y_ = true;
    bool l_ = true;
    bool r_ = true;


}; // Gamepad
