#include <thread>
#include <chrono>

#include "gamepad.h"

void Gamepad::loop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));        
    }
}

void Gamepad::initializeDevice() {
        struct libevdev * dev = libevdev_new();
        libevdev_set_name(dev, "rcboy gamepad");
        libevdev_set_id_bustype(dev, BUS_USB);
        libevdev_set_id_vendor(dev, 0xcafe);
        libevdev_set_id_product(dev, 0xbabe);
        // enable keys for the buttons
        libevdev_enable_event_type(dev, EV_KEY);
        libevdev_enable_event_code(dev, EV_KEY, static_cast<unsigned>(Button::A), nullptr);
        libevdev_enable_event_code(dev, EV_KEY, static_cast<unsigned>(Button::B), nullptr);
        libevdev_enable_event_code(dev, EV_KEY, static_cast<unsigned>(Button::X), nullptr);
        libevdev_enable_event_code(dev, EV_KEY, static_cast<unsigned>(Button::Y), nullptr);
        libevdev_enable_event_code(dev, EV_KEY, static_cast<unsigned>(Button::Left), nullptr);
        libevdev_enable_event_code(dev, EV_KEY, static_cast<unsigned>(Button::Right), nullptr);
        libevdev_enable_event_code(dev, EV_KEY, static_cast<unsigned>(Button::Select), nullptr);
        libevdev_enable_event_code(dev, EV_KEY, static_cast<unsigned>(Button::Start), nullptr);
        // enable the thumbstick and accelerometer
        libevdev_enable_event_type(dev, EV_ABS);
        input_absinfo info {
            .value = 128,
            .minimum = 0,
            .maximum = 255,
            .fuzz = 0,
            .flat = 0,
            .resolution = 1,
        };
        libevdev_enable_event_code(dev, EV_ABS, static_cast<unsigned>(Axis::ThumbX), & info);
        libevdev_enable_event_code(dev, EV_ABS, static_cast<unsigned>(Axis::ThumbY), & info);
        libevdev_enable_event_code(dev, EV_ABS, static_cast<unsigned>(Axis::AccelX), & info);
        libevdev_enable_event_code(dev, EV_ABS, static_cast<unsigned>(Axis::AccelY), & info);
        libevdev_enable_event_code(dev, EV_ABS, static_cast<unsigned>(Axis::AccelZ), & info);
        int err = libevdev_uinput_create_from_device(dev,
                                                LIBEVDEV_UINPUT_OPEN_MANAGED,
                                                &uidev_);
        if (err != 0)
            std::cout << "cannot do what I want to do" << std::endl;
        libevdev_free(dev);

}

void Gamepad::initializeGPIO() {
    gpioInitialise();
    gpioSetMode(GPIO_A, PI_INPUT);
    gpioSetPullUpDown(GPIO_A, PI_PUD_UP);
    gpioSetISRFuncEx(GPIO_A, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnA, this);
    gpioSetMode(GPIO_B, PI_INPUT);
    gpioSetPullUpDown(GPIO_B, PI_PUD_UP);
    gpioSetISRFuncEx(GPIO_B, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnB, this);
    gpioSetMode(GPIO_X, PI_INPUT);
    gpioSetPullUpDown(GPIO_X, PI_PUD_UP);
    gpioSetISRFuncEx(GPIO_X, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnX, this);
    gpioSetMode(GPIO_Y, PI_INPUT);
    gpioSetPullUpDown(GPIO_Y, PI_PUD_UP);
    gpioSetISRFuncEx(GPIO_Y, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnY, this);
}

void Gamepad::initializeI2C() {

}


void Gamepad::isrBtnA(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->a_ != level) {
        gamepad->a_ = level;
        gamepad->a_ ? gamepad->buttonRelease(Button::A) : gamepad->buttonPress(Button::A);
    }
}

void Gamepad::isrBtnB(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->b_ != level) {
        gamepad->b_ = level;
        gamepad->b_ ? gamepad->buttonRelease(Button::B) : gamepad->buttonPress(Button::B);
    }
}
void Gamepad::isrBtnX(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->x_ != level) {
        gamepad->x_ = level;
        gamepad->x_ ? gamepad->buttonRelease(Button::X) : gamepad->buttonPress(Button::X);
    }
}

void Gamepad::isrBtnY(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->y_ != level) {
        gamepad->y_ = level;
        gamepad->y_ ? gamepad->buttonRelease(Button::Y) : gamepad->buttonPress(Button::Y);
    }
}
