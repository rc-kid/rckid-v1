#include "rcboy.h"

//#include <wiringPi.h>

#include <iostream>

void handle() {
    std::cout << "here" << std::endl;
}


RCBoy * RCBoy::initialize() {
/*
   wiringPiSetupGpio();
	pinMode(PIN_BTN_A, INPUT);
    pullUpDnControl(PIN_BTN_A, PUD_UP);

	// Bind to interrupt
	wiringPiISR(PIN_BTN_A, INT_EDGE_BOTH, &handle);


    return nullptr;
*/

    gpio::initialize();
    spi::initialize();
    i2c::initializeMaster();

    RCBoy * result = new RCBoy{};
    result->initializeLibevdevGamepad();
    result->initializeAccel();
    result->initializeNrf();
    result->initializeISRs();

    // start the hw loop thread
    std::thread t{[result](){
        result->hwLoop();
    }};
    t.detach();
    std::thread tickTimer{[result](){
        while (true) {
            cpu::delay_ms(10);
            result->hwEvents_.send(HWEvent::Tick);
        }
    }};
    tickTimer.detach();
    return result;
}


RCBoy::RCBoy() {
    
}
 
void RCBoy::hwLoop() {
    // query avr status
    avrQueryStatus();

    while (true) {
        HWEvent e = hwEvents_.waitReceive();
        switch (e) {
            case HWEvent::Tick: {
                for (Button & b : buttons_)
                    buttonTick(&b);
                break;
            }
            case HWEvent::AvrIrq: {
                avrQueryStatus();
                break;
            }
        }
    }
}

void RCBoy::avrQueryStatus() {
    comms::Status status;
    i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& status, sizeof(status));
    processAvrStatus(status);
}

void RCBoy::avrQueryFullState() {
    comms::FullState state;
    i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
}

void RCBoy::processAvrStatus(comms::Status const & status) {
    //std::cout << (int) *((uint8_t*) & status) << std::endl;
 
    if (buttons_[0].current != status.btnVolumeLeft())
        isrButton(gpio::UNUSED, !status.btnVolumeLeft(), 0, & buttons_[0]);
    if (buttons_[1].current != status.btnVolumeRight())
        isrButton(gpio::UNUSED, !status.btnVolumeRight(), 0, & buttons_[1]);
    // TODO joystik button is missing
    
    /*
    bool volLeftChanged = volumeLeft_.update(status.btnVolumeLeft());
    bool volRightChanged = volumeRight_.update(status.btnVolumeRight());
    bool thumbPosChanged = thumbX_.update(status.joyX());
    thumbPosChanged = thumbY_.update(status.joyY()) || thumbPosChanged;

    bool chrgChanged = charging_ != status.charging();
    charging_ = status.charging();
    bool lowBattChanged = lowBattery_ != status.lowBattery();
    lowBattery_ = status.lowBattery();
    */


}


void RCBoy::initializeISRs() {
    gpio::inputPullup(PIN_AVR_IRQ);
    gpio::input(PIN_HEADPHONES);
    for (Button & b : buttons_) {
        if (b.pin != gpio::UNUSED) {
            gpio::inputPullup(b.pin);
            gpioSetISRFuncEx(b.pin, EITHER_EDGE, 0, (gpioISRFuncEx_t) RCBoy::isrButton, & b);
        }
    }
    gpioSetISRFuncEx(PIN_AVR_IRQ, FALLING_EDGE, 0,  (gpioISRFuncEx_t) RCBoy::isrAvrIrq, this);
    //gpioSetISRFuncEx(PIN_HEADPHONES, EITHER_EDGE, 0, (gpioISRFuncEx_t) Driver::isrHeadphonesChange, this);
    /*

    gpio::inputPullup(PIN_BTN_A);
    gpio::inputPullup(PIN_BTN_B);
    gpio::inputPullup(PIN_BTN_X);
    gpio::inputPullup(PIN_BTN_Y);
    gpio::inputPullup(PIN_BTN_L);
    gpio::inputPullup(PIN_BTN_R);
    gpio::inputPullup(PIN_BTN_SELECT);
    gpio::inputPullup(PIN_BTN_START);
#if (defined ARCH_RPI)
    gpioSetISRFuncEx(PIN_BTN_A, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrButtonChange, & a_);
    gpioSetISRFuncEx(PIN_BTN_B, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrButtonChange, & b_);
    gpioSetISRFuncEx(PIN_BTN_X, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrButtonChange, & x_);
    gpioSetISRFuncEx(PIN_BTN_Y, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrButtonChange, & y_);
    gpioSetISRFuncEx(PIN_BTN_L, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrButtonChange, & l_);
    gpioSetISRFuncEx(PIN_BTN_R, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrButtonChange, & r_);
    gpioSetISRFuncEx(PIN_BTN_SELECT, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrButtonChange, & select_);
    gpioSetISRFuncEx(PIN_BTN_START, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrButtonChange, & start_);
    gpioSetISRFuncEx(PIN_AVR_IRQ, FALLING_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrAvrIrq, this);
    gpioSetISRFuncEx(PIN_HEADPHONES, EITHER_EDGE, 0, (gpioISRFuncEx_t) Driver::isrHeadphonesChange, this);
#endif
    */
}

void RCBoy::initializeLibevdevGamepad() {
    struct libevdev * dev = libevdev_new();
    libevdev_set_name(dev, LIBEVDEV_DEVICE_NAME);
    libevdev_set_id_bustype(dev, BUS_USB);
    libevdev_set_id_vendor(dev, 0x0ada);
    libevdev_set_id_product(dev, 0xbabe);
    // enable keys for the buttons
    libevdev_enable_event_type(dev, EV_KEY);
    for (Button & b : buttons_)
        libevdev_enable_event_code(dev, EV_KEY, b.evdevId, nullptr);
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

/*
    libevdev_enable_event_code(dev, EV_KEY, dpadUp_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, dpadDown_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, dpadLeft_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, dpadRight_.evdevId, nullptr);
    */

    int err = libevdev_uinput_create_from_device(dev,
                                            LIBEVDEV_UINPUT_OPEN_MANAGED,
                                            &uidev_);
    // TODO where to report this error???? ANd how toreport errors in general, restart avr?
    //if (err != 0)
    //    std::cout << "cannot do what I want to do: " << err << std::endl;
    libevdev_free(dev);
}

void RCBoy::initializeAccel() {
    if (accel_.deviceIdentification() == 104) {
        accel_.reset();
    } else {
        ERROR("Accel not found");
    }
}

void RCBoy::initializeNrf() {
    if (radio_.initialize("TEST1", "TEST2")) {
        radio_.standby();
    } else {
        ERROR("Radio not found");
    }
}



