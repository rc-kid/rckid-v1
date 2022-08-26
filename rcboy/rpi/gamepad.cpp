#include <iostream>

#include "gamepad.h"

#include "comms.h"
#include "log.h"

using namespace std::chrono_literals;


void Gamepad::loop() {
    auto last = std::chrono::system_clock::now();        
    auto nextAccel = last + 5ms;
    auto nextAvr = last + 1s;
    while (true) {
        {
            std::unique_lock g{m_};
            cv_.wait_until(g, nextAccel, [] () { return q_.empty(); });
            while (! q_.empty()) {
                switch (q_.front()) {
                    case Event::AVR_IRQ:
                        g.unlock();
                        nextAvr = std::chrono::system_clock::now() + 1s;
                        queryAvr();
                        g.lock();
                        break;
                }
                q_.pop_front();
            }
        }
        last = std::chrono::system_clock::now();
        if (last >= nextAccel) {
            nextAccel = last + 5ms;
            queryAccelerometer();
        }
        if (last >= nextAvr) {
            // if there's been enough ticks, talk to AVR so that it knows we are still alive (once per second)
            nextAvr = last + 1s;
            queryAvr();
        }
    }
}

/** THe AVR is queried when the irq pin is pulled low or at least every second even without the pull as a form of watchdog. 
 */
void Gamepad::queryAvr() {
    comms::Status state;
    i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(comms::Status));
    //std::cout << (int)state.status_ << " " << (int)state.joyX_ << " " << (int)state.joyY_ << " " << (int)state.photores_ << std::endl;
    // once we have the state, check the buttons, axes and other settings
    if (volumeLeft_.state != state.btnVolumeLeft())
        isrButtonChange(0, volumeLeft_.state, 0, & volumeLeft_);
    if (volumeRight_.state != state.btnVolumeRight())
        isrButtonChange(0, volumeRight_.state, 0, & volumeRight_);
    if (thumbBtn_.state != state.btnJoystick())
        isrButtonChange(0, thumbBtn_.state, 0, & thumbBtn_);

/*
    if (sel_ != state.btnSelect()) {
        sel_ = state.btnSelect();
        buttonChange(Button::Select, ! sel_);
    }
    if (start_ != state.btnStart()) {
        start_ = state.btnStart();
        buttonChange(Button::Start, ! start_);
    } */
    /*
    if (thumbX_ != state.joyX()) {
        thumbX_ = state.joyX();
        axisChange(Axis::ThumbX, thumbX_);
    }
    if (thumbY_ != state.joyY()) {
        thumbY_ = state.joyY();
        axisChange(Axis::ThumbY, thumbY_);
    } */
}

void Gamepad::queryAccelerometer() {
    MPU6050::AccelData d = accel_.readAccel();
    d.toUnsignedByte();
    std::cout << "X: " << d.x << ", Y: " << d.y << " Z: " << d.z << std::endl;
    // also read the temperature so that we have one more datapoint whether the handheld overheats or not
    uint16_t t = accel_.readTemp();
}


void Gamepad::initializeLibevdevDevice() {
        struct libevdev * dev = libevdev_new();
        libevdev_set_name(dev, DEVICE_NAME);
        libevdev_set_id_bustype(dev, BUS_USB);
        libevdev_set_id_vendor(dev, 0x0ada);
        libevdev_set_id_product(dev, 0xbabe);
        // enable keys for the buttons
        libevdev_enable_event_type(dev, EV_KEY);
        libevdev_enable_event_code(dev, EV_KEY, a_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, b_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, x_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, y_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, l_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, r_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, select_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, start_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, volumeLeft_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, volumeRight_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, thumbBtn_.evdevId, nullptr);
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
        libevdev_enable_event_code(dev, EV_ABS, thumbX_.evdevId, & info);
        libevdev_enable_event_code(dev, EV_ABS, thumbY_.evdevId, & info);
        libevdev_enable_event_code(dev, EV_ABS, accelX_.evdevId, & info);
        libevdev_enable_event_code(dev, EV_ABS, accelY_.evdevId, & info);
        libevdev_enable_event_code(dev, EV_ABS, accelZ_.evdevId, & info);
        int err = libevdev_uinput_create_from_device(dev,
                                                LIBEVDEV_UINPUT_OPEN_MANAGED,
                                                &Gamepad::uidev_);
//        if (err != 0)
//            std::cout << "cannot do what I want to do" << std::endl;
        libevdev_free(dev);

}

void Gamepad::initializePins() {
    gpio::inputPullup(GPIO_A);
    gpio::inputPullup(GPIO_B);
    gpio::inputPullup(GPIO_X);
    gpio::inputPullup(GPIO_Y);
    gpio::inputPullup(GPIO_L);
    gpio::inputPullup(GPIO_R);
    gpio::inputPullup(GPIO_SELECT);
    gpio::inputPullup(GPIO_START);
    gpio::inputPullup(GPIO_AVR_IRQ);
#if (defined ARCH_RPI)
    gpioSetISRFuncEx(GPIO_A, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrButtonChange, & a_);
    gpioSetISRFuncEx(GPIO_B, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrButtonChange, & b_);
    gpioSetISRFuncEx(GPIO_X, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrButtonChange, & x_);
    gpioSetISRFuncEx(GPIO_Y, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrButtonChange, & y_);
    gpioSetISRFuncEx(GPIO_L, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrButtonChange, & l_);
    gpioSetISRFuncEx(GPIO_R, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrButtonChange, & r_);
    gpioSetISRFuncEx(GPIO_SELECT, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrButtonChange, & select_);
    gpioSetISRFuncEx(GPIO_START, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrButtonChange, & start_);
    gpioSetISRFuncEx(GPIO_AVR_IRQ, FALLING_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrAvrIrq, nullptr);
#endif

}

void Gamepad::initializeI2C() {
    i2c::initializeMaster();
    cpu::delay_ms(5);
    accel_.reset();
    std::cout << (int)accel_.deviceIdentification() << std::endl;
}