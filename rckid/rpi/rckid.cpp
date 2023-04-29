#include <iostream>

#include "window.h"
#include "rckid.h"

#if (!defined ARCH_RPI)
#undef KEY_APOSTROPHE
#undef KEY_COMMA
#undef KEY_MINUS
#undef KEY_SLASH
#undef KEY_SEMICOLON
#undef KEY_EQUAL
#undef KEY_A
#undef KEY_B
#undef KEY_C
#undef KEY_D
#undef KEY_E
#undef KEY_F
#undef KEY_G
#undef KEY_H
#undef KEY_I
#undef KEY_J
#undef KEY_K
#undef KEY_L
#undef KEY_M
#undef KEY_N
#undef KEY_O
#undef KEY_P
#undef KEY_Q
#undef KEY_R
#undef KEY_S
#undef KEY_T
#undef KEY_U
#undef KEY_V
#undef KEY_W
#undef KEY_X
#undef KEY_Y
#undef KEY_Z
#undef KEY_BACKSLASH
#undef KEY_GRAVE
#undef KEY_SPACE
#undef KEY_ENTER
#undef KEY_TAB
#undef KEY_BACKSPACE
#undef KEY_INSERT
#undef KEY_DELETE
#undef KEY_RIGHT
#undef KEY_LEFT
#undef KEY_DOWN
#undef KEY_UP
#undef KEY_HOME
#undef KEY_END
#undef KEY_PAUSE
#undef KEY_F1
#undef KEY_F2
#undef KEY_F3
#undef KEY_F4
#undef KEY_F5
#undef KEY_F6
#undef KEY_F7
#undef KEY_F8
#undef KEY_F9
#undef KEY_F10
#undef KEY_F11
#undef KEY_F12
#undef KEY_BACK
#undef KEY_MENU
#include <raylib.h>
#endif


using namespace platform;

RCKid::RCKid(Window * window): 
    window_{window} {
    gpio::initialize();
    if (!spi::initialize()) 
        ERROR("Unable to initialize spi (errno " << errno << ")");
    if (!i2c::initializeMaster())
        ERROR("Unable to initialize i2c (errno " << errno << "), make sure /dev/i2c1 exists");
    RCKid * & i = instance();
    ASSERT(i == nullptr && "RCKid must be a singleton");
    i = this;
    initializeLibevdevGamepad();
    initializeAvr();
    initializeAccel();
    initializeNrf();
    initializeISRs();

    // start the hw loop thread
    std::thread t{[this](){
        this->hwLoop();
    }};
    t.detach();
    std::thread tickTimer{[this](){
        while (true) {
            cpu::delay_ms(10);
            this->hwEvents_.send(HWEvent::Tick);
        }
    }};
    tickTimer.detach();
}

void RCKid::hwLoop() {
    // query avr status
    avrQueryState();
    setBrightness(255);
    while (true) {
        HWEvent e = hwEvents_.waitReceive();
        switch (e) {
            case HWEvent::Tick: {
#if (defined ARCH_MOCK)        
                checkMockButtons();
#endif
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
                buttonTick(btnDpadLeft_);
                buttonTick(btnDpadRight_);
                buttonTick(btnDpadUp_);
                buttonTick(btnDpadDown_);
                buttonTick(btnJoy_);
                buttonTick(btnHome_);
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

#if (defined ARCH_MOCK)
void RCKid::checkMockButtons() {
#define CHECK_RPI_KEY(KEY, VALUE) if (IsKeyDown(KEY) != VALUE.current) buttonChange(VALUE.current, VALUE);
#define CHECK_AVR_KEY(KEY, VALUE) if (IsKeyDown(KEY) != VALUE.current) buttonChange(VALUE.current, VALUE);
    CHECK_RPI_KEY(KeyboardKey::KEY_A, btnA_);
    CHECK_RPI_KEY(KeyboardKey::KEY_B, btnB_);
    CHECK_RPI_KEY(KeyboardKey::KEY_X, btnX_);
    CHECK_RPI_KEY(KeyboardKey::KEY_Y, btnY_);
    CHECK_RPI_KEY(KeyboardKey::KEY_L, btnL_);
    CHECK_RPI_KEY(KeyboardKey::KEY_R, btnR_);
    CHECK_RPI_KEY(KeyboardKey::KEY_COMMA, btnVolDown_);
    CHECK_RPI_KEY(KeyboardKey::KEY_PERIOD, btnVolUp_);
    CHECK_RPI_KEY(KeyboardKey::KEY_D, btnJoy_);
    
    CHECK_AVR_KEY(KeyboardKey::KEY_ENTER, btnSelect_);
    CHECK_AVR_KEY(KeyboardKey::KEY_SPACE, btnStart_);
    CHECK_AVR_KEY(KeyboardKey::KEY_LEFT, btnDpadLeft_);
    CHECK_AVR_KEY(KeyboardKey::KEY_RIGHT, btnDpadRight_);
    CHECK_AVR_KEY(KeyboardKey::KEY_UP, btnDpadUp_);
    CHECK_AVR_KEY(KeyboardKey::KEY_DOWN, btnDpadDown_);
    CHECK_AVR_KEY(KeyboardKey::KEY_H, btnHome_);
#undef CHECK_RPI_KEY
}
#endif

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
    if (btnDpadUp_.current != controls.dpadUp())
        buttonChange(!controls.dpadUp(), btnDpadUp_);
    if (btnDpadDown_.current != controls.dpadDown())
        buttonChange(!controls.dpadDown(), btnDpadDown_);

    if (btnSelect_.current != controls.select())
        buttonChange(!controls.select(), btnSelect_);
    if (btnStart_.current != controls.start())
        buttonChange(!controls.start(), btnStart_);
    if (btnHome_.current != controls.home())
        buttonChange(!controls.home(), btnHome_);

    // TODO reenable when done

    //axisChange(controls.joyH(), thumbX_);
    //axisChange(controls.joyV(), thumbY_);
    
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
    gpio::attachInterrupt(PIN_BTN_JOY, gpio::Edge::Both, & isrButtonJoy);

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
    // enable keys for retroarch and vlc shortcuts
    libevdev_enable_event_code(dev, EV_KEY, RETROARCH_PAUSE, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, RETROARCH_SAVE_STATE, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, RETROARCH_LOAD_STATE, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, RETROARCH_SCREENSHOT, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, VLC_PAUSE, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, VLC_BACK, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, VLC_FORWARD, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, VLC_DELAY_10S, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, VLC_DELAY_1M, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, VLC_SCREENSHOT, nullptr);
    libevdev_enable_event_code(dev, EV_KEY, VLC_SCREENSHOT_MOD, nullptr);
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

void RCKid::buttonAction(ButtonState & btn) ISR_THREAD DRIVER_THREAD {
    btn.reported = btn.current;
    btn.autorepeat = BTN_AUTOREPEAT_DURATION;
    if (uidev_ != nullptr) {
        libevdev_uinput_write_event(uidev_, EV_KEY, btn.evdevId, btn.reported ? 1 : 0);
        libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
    }
    // send the appropriate action to the main thread
    window_->send(Event::button(btn.button, btn.reported));
}



