
#include "gamepad.h"

using namespace std::chrono_literals;

void Gamepad::loop() {
    auto last = std::chrono::system_clock::now();        
    auto next = last + 5ms;
    size_t ticks = 0;
    while (true) {
        {
            std::unique_lock g(m_);
            cv_.wait_until(g, next, [this] () { return q_.empty(); });
            while (! q_.empty()) {
                switch (q_.front()) {
                    case Event::ButtonAChange:
                        buttonChange(Button::A, ! a_);
                        break;
                    case Event::ButtonBChange:
                        buttonChange(Button::B, ! b_);
                        break;
                    case Event::ButtonXChange:
                        buttonChange(Button::X, ! x_);
                        break;
                    case Event::ButtonYChange:
                        buttonChange(Button::Y, ! y_);
                        break;
                    case Event::ButtonLeftChange:
                        buttonChange(Button::Left, ! l_);
                        break;
                    case Event::ButtonRightChange:
                        buttonChange(Button::Right, ! r_);
                        break;
                    case Event::AvrIrq: 
                        g.unlock(); // this will take time, so release the lock fpor other interrupts
                        queryAVR();
                        g.lock();
                        ticks = 0; // reset ticks for the heartbeat
                }
                q_.pop_front();
            }
        }
        last = std::chrono::system_clock::now();
        if (last >= next) {
            queryAccelerometer();
            // if there's been enough ticks, talk to AVR so that it knows we are still alive (once per second)
            if (++ticks >=200) {
                queryAVR();
                ticks = 0;
            }
        }
    }
}

void Gamepad::queryAccelerometer() {
    MPU6050::AccelData d = accel_.readAccel();
    d.toUnsignedByte();
    //std::cout << "X: " << d.x << ", Y: " << d.y << " Z: " << d.z << std::endl;
    // also read the temperature so that we have one more datapoint whether the handheld overheats or not
    uint16_t t = accel_.readTemp();

}

/** TODO actuallytalk to the AVR. 
 */
void Gamepad::queryAVR() {
    // TODO !
}

void Gamepad::isrAvrIrq(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    gamepad->addEvent(Gamepad::Event::AvrIrq);
}

void Gamepad::initializeDevice() {
        struct libevdev * dev = libevdev_new();
        libevdev_set_name(dev, "rcboy gamepad");
        libevdev_set_id_bustype(dev, BUS_USB);
        libevdev_set_id_vendor(dev, 0x0ada);
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
    gpio::inputPullup(GPIO_A);
    gpio::inputPullup(GPIO_B);
    gpio::inputPullup(GPIO_X);
    gpio::inputPullup(GPIO_Y);
    gpio::inputPullup(GPIO_L);
    gpio::inputPullup(GPIO_R);
    gpio::inputPullup(GPIO_AVR_IRQ);
#if (defined ARCH_RPI)
    gpioSetISRFuncEx(GPIO_A, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnA, this);
    gpioSetISRFuncEx(GPIO_B, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnB, this);
    gpioSetISRFuncEx(GPIO_X, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnX, this);
    gpioSetISRFuncEx(GPIO_Y, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnY, this);
    gpioSetISRFuncEx(GPIO_L, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnL, this);
    gpioSetISRFuncEx(GPIO_R, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrBtnR, this);
    gpioSetISRFuncEx(GPIO_AVR_IRQ, FALLING_EDGE, 0,  (gpioISRFuncEx_t) Gamepad::isrAvrIrq, this);
#endif
}

void Gamepad::initializeI2C() {
    i2c::initialize();
    cpu::delay_ms(5);
    accel_.reset();
}

void Gamepad::isrBtnA(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->a_ != level) {
        gamepad->a_ = level;
        gamepad->addEvent(Gamepad::Event::ButtonAChange);
    }
}

void Gamepad::isrBtnB(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->b_ != level) {
        gamepad->b_ = level;
        gamepad->addEvent(Gamepad::Event::ButtonBChange);
    }
}

void Gamepad::isrBtnX(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->x_ != level) {
        gamepad->x_ = level;
        gamepad->addEvent(Gamepad::Event::ButtonXChange);
    }
}

void Gamepad::isrBtnY(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->y_ != level) {
        gamepad->y_ = level;
        gamepad->addEvent(Gamepad::Event::ButtonYChange);
    }
}

void Gamepad::isrBtnL(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->l_ != level) {
        gamepad->l_ = level;
        gamepad->addEvent(Gamepad::Event::ButtonLeftChange);
    }
}

void Gamepad::isrBtnR(int gpio, int level, uint32_t tick, Gamepad * gamepad) {
    if (gamepad->r_ != level) {
        gamepad->r_ = level;
        gamepad->addEvent(Gamepad::Event::ButtonRightChange);
    }
}
