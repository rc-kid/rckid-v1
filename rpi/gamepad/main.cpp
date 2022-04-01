
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>


#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/uinput.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include <pigpio.h>

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-uinput.h"


class Gamepad {
public:
    Gamepad() {
        dev_ = libevdev_new();
        libevdev_set_name(dev_, "rcboy gamepad");
        libevdev_set_id_bustype(dev_, BUS_USB);
        libevdev_set_id_vendor(dev_, 0xcafe);
        libevdev_set_id_product(dev_, 0xbabe);
        // enable keys for the buttons
        libevdev_enable_event_type(dev_, EV_KEY);
        libevdev_enable_event_code(dev_, EV_KEY, BTN_A, nullptr);
        libevdev_enable_event_code(dev_, EV_KEY, BTN_B, nullptr);
        libevdev_enable_event_code(dev_, EV_KEY, BTN_X, nullptr);
        libevdev_enable_event_code(dev_, EV_KEY, BTN_Y, nullptr);
        libevdev_enable_event_code(dev_, EV_KEY, BTN_LEFT, nullptr);
        libevdev_enable_event_code(dev_, EV_KEY, BTN_RIGHT, nullptr);
        libevdev_enable_event_code(dev_, EV_KEY, BTN_START, nullptr);
        libevdev_enable_event_code(dev_, EV_KEY, BTN_SELECT, nullptr);
        // enable the thumbstick and accelerometer
        libevdev_enable_event_type(dev_, EV_ABS);
        input_absinfo info {
            .value = 128,
            .minimum = 0,
            .maximum = 255,
            .fuzz = 0,
            .flat = 0,
            .resolution = 1,
        };
        libevdev_enable_event_code(dev_, EV_ABS, ABS_RX, & info);
        libevdev_enable_event_code(dev_, EV_ABS, ABS_RY, & info);
        libevdev_enable_event_code(dev_, EV_ABS, ABS_X, & info);
        libevdev_enable_event_code(dev_, EV_ABS, ABS_Y, & info);
        libevdev_enable_event_code(dev_, EV_ABS, ABS_Z, & info);
        int err = libevdev_uinput_create_from_device(dev_,
                                                LIBEVDEV_UINPUT_OPEN_MANAGED,
                                                &uidev_);
        if (err != 0)
            std::cout << "cannot do what I want to do" << std::endl;
        libevdev_free(dev_);
    }

    ~Gamepad() {
        libevdev_uinput_destroy(uidev_);
    }

private:
    struct libevdev *dev_;
    struct libevdev_uinput *uidev_;

}; // Gamepad

void mine() {
    //int fd = open("/dev/uinput", O_RDWR | O_NONBLOCK);
    struct libevdev *dev;
    struct libevdev_uinput *uidev;
    for (int i = 0; i < 120; ++i) {
        libevdev_uinput_write_event(uidev, EV_ABS, ABS_RX, i);
        libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
        sleep(1);
    }



    // ... do something ...
    //close(fd);

}





int main(int argc, char * argv[]) {
    mine();
    //ex();
    return EXIT_SUCCESS;
}