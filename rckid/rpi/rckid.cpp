#include "rckid.h"

#include <iostream>

using namespace platform;

RCKid * RCKid::initialize() {
    gpio::initialize();
    if (!spi::initialize()) 
        ERROR("Unable to initialize spi (errno " << errno << ")");
    if (!i2c::initializeMaster())
        ERROR("Unable to initialize i2c (errno " << errno << "), make sure /dev/i2c1 exists");
    RCKid * result = instance();
    result->initializeLibevdevGamepad();
    result->initializeAvr();
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


RCKid::RCKid() {
    
}
 
void RCKid::hwLoop() {
    // query avr status
    avrQueryState();
    while (true) {
        HWEvent e = hwEvents_.waitReceive();
        switch (e) {
            case HWEvent::Tick: {
                buttonTick(btnVolDown_);
                buttonTick(btnVolUp_);
                buttonTick(btnA_);
                buttonTick(btnB_);
                buttonTick(btnX_);
                buttonTick(btnY_);
                buttonTick(btnL_);
                buttonTick(btnR_);
                buttonTick(btnSelect_);
                buttonTick(btnStart_);
                accelQueryStatus();
                break;
            }
            case HWEvent::AvrIrq: {
                avrQueryState();
                break;
            }
        }
    }
}

uint8_t accelTo1GUnsigned(int16_t v) {
    if (v < -16384)
        v = -16384;
    if (v >= 16384)
        v = 16383;
    v += 16384;
    return (v >> 7);    
}

void RCKid::accelQueryStatus() {
    MPU6050::AccelData d = accel_.readAccel();
    uint16_t t = accel_.readTemp();
    //axisChange(accelTo1GUnsigned(-d.x), accelX_);
    //axisChange(accelTo1GUnsigned(-d.y), accelY_);
}

void RCKid::avrQueryState() {
    comms::State state;
    i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
    processAvrStatus(state.status);
    processAvrControls(state.controls);
}

void RCKid::avrQueryExtendedState() {
    comms::ExtendedState state;
    i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
}

void RCKid::processAvrStatus(comms::Status const & status) {

}

void RCKid::processAvrControls(comms::Controls const & controls) {
    //std::cout << (int) *((uint8_t*) & status) << std::endl;

    if (btnDpadLeft_.current != controls.dpadLeft())
        buttonChange(!controls.dpadLeft(), btnDpadLeft_);
    if (btnDpadRight_.current != controls.dpadRight())
        buttonChange(!controls.dpadRight(), btnDpadRight_);
    if (btnDpadUp_.current != controls.dpadTop())
        buttonChange(!controls.dpadTop(), btnDpadUp_);
    if (btnDpadDown_.current != controls.dpadBottom())
        buttonChange(!controls.dpadBottom(), btnDpadDown_);

    if (btnSelect_.current != controls.select())
        buttonChange(!controls.select(), btnSelect_);
    if (btnStart_.current != controls.start())
        buttonChange(!controls.start(), btnStart_);
    if (btnHome_.current != controls.home())
        buttonChange(!controls.home(), btnHome_);

    axisChange(controls.joyH(), thumbX_);
    axisChange(controls.joyV(), thumbY_);
    
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


void RCKid::initializeISRs() {
    gpio::inputPullup(PIN_AVR_IRQ);
    gpio::attachInterrupt(PIN_AVR_IRQ, gpio::Edge::Falling, & isrAvrIrq);

    gpio::input(PIN_HEADPHONES);
    gpio::attachInterrupt(PIN_HEADPHONES, gpio::Edge::Both, & isrHeadphones);

    gpio::inputPullup(PIN_BTN_A);
    gpio::inputPullup(PIN_BTN_B);
    gpio::inputPullup(PIN_BTN_X);
    gpio::inputPullup(PIN_BTN_Y);
    gpio::inputPullup(PIN_BTN_L);
    gpio::inputPullup(PIN_BTN_R);
    gpio::inputPullup(PIN_BTN_LVOL);
    gpio::inputPullup(PIN_BTN_RVOL);
    gpio::attachInterrupt(PIN_BTN_A, gpio::Edge::Both, & isrButtonA);
    gpio::attachInterrupt(PIN_BTN_B, gpio::Edge::Both, & isrButtonB);
    gpio::attachInterrupt(PIN_BTN_X, gpio::Edge::Both, & isrButtonX);
    gpio::attachInterrupt(PIN_BTN_Y, gpio::Edge::Both, & isrButtonY);
    gpio::attachInterrupt(PIN_BTN_L, gpio::Edge::Both, & isrButtonL);
    gpio::attachInterrupt(PIN_BTN_R, gpio::Edge::Both, & isrButtonR);
    gpio::attachInterrupt(PIN_BTN_LVOL, gpio::Edge::Both, & isrButtonLVol);
    gpio::attachInterrupt(PIN_BTN_RVOL, gpio::Edge::Both, & isrButtonRVol);

    //gpioSetISRFuncEx(PIN_AVR_IRQ, FALLING_EDGE, 0,  (gpioISRFuncEx_t) RCKid::isrAvrIrq, this);
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

void RCKid::initializeLibevdevGamepad() {
    struct libevdev * dev = libevdev_new();
    libevdev_set_name(dev, LIBEVDEV_DEVICE_NAME);
    libevdev_set_id_bustype(dev, BUS_USB);
    libevdev_set_id_vendor(dev, 0x0ada);
    libevdev_set_id_product(dev, 0xbabe);
    // enable keys for the buttons
    libevdev_enable_event_type(dev, EV_KEY);
    libevdev_enable_event_code(dev, EV_KEY, btnVolDown_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnVolUp_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnA_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnB_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnX_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnY_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnL_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnR_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnSelect_.evdevId, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, btnStart_.evdevId, nullptr);
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
    if (err != 0) 
        ERROR("Unable to setup gamepad (result " << err << ", errno: " << errno << ")");
    libevdev_free(dev);
}

void RCKid::initializeAvr() {
    if (!i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, nullptr, 0))
        ERROR("AVR not found:" << errno);
}

void RCKid::initializeAccel() {
    if (accel_.deviceIdentification() == 104) {
        accel_.reset();
    } else {
        ERROR("Accel not found");
    }
}

void RCKid::initializeNrf() {
    if (radio_.initialize("TEST1", "TEST2")) {
        radio_.standby();
    } else {
        ERROR("Radio not found");
    }
}



