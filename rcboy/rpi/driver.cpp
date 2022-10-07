#include "log.h"
#include "driver.h"

using namespace std::chrono_literals;

constexpr auto AVR_MAX_PERIOD = 1s;
constexpr auto ACCEL_MAX_PERIOD = 5ms;
static constexpr char const * DEVICE_NAME = "rcboy-gamepad";


bool Driver::Button::update(bool newState) {
    // TODO debounce
    if (state != newState) {
        state = newState;
        if (evdevId != KEY_RESERVED && singleton_->uidev_ != nullptr) {
            libevdev_uinput_write_event(singleton_->uidev_, EV_KEY, evdevId, state ? 1 : 0);
            libevdev_uinput_write_event(singleton_->uidev_, EV_SYN, SYN_REPORT, 0);
        }
        return true;
    } else {
        return false;
    }
}

Driver::TriState Driver::AnalogButton::update(uint8_t value) {
    bool changed = false;
    if (state) {
        if (overThresholdOff(value)) {
            state = false;
            changed = true;
        }
    } else {
        if (overThresholdOn(value)) {
            state = true;
            changed = true;
        }
    } 
    if (changed && evdevId != KEY_RESERVED && singleton_->uidev_ != nullptr) {
        libevdev_uinput_write_event(singleton_->uidev_, EV_KEY, evdevId, state ? 1 : 0);
        libevdev_uinput_write_event(singleton_->uidev_, EV_SYN, SYN_REPORT, 0);
    }
    if (changed)
        return state ? TriState::On : TriState::Off;
    else
        return TriState::Unchanged;
}

Driver * Driver::initialize() {
    LOG("Initializing...");
    LOG("  gpio");
    gpio::initialize();
    LOG("  spi");
    spi::initialize();
    LOG("  i2c");
    i2c::initializeMaster();
    // create the singleton driver
    singleton_ = new Driver{};
    LOG("  gamepad");
    singleton_->initializeLibevdevDevice();
    singleton_->initializePins();
    //initialize the peripherals
    LOG("  avr");
    // TODO
    LOG("  accel");
    if (singleton_->accel_.deviceIdentification() != 104)
        ERROR("Accel not found");
    singleton_->accel_.reset();
    LOG("  radio");
    if (!singleton_->radio_.initialize("TEST1", "TEST2")) 
        ERROR("Radio not found");
    singleton_->radio_.standby();
    gpioSetISRFuncEx(PIN_NRF_IRQ, EITHER_EDGE, 0,  (gpioISRFuncEx_t) Driver::isrRadio, singleton_);

    LOG("  driver thread");
    std::thread t{[](){
        singleton_->loop();
    }};
    t.detach();

    return singleton_;
}

void Driver::loop() {
    auto last = std::chrono::system_clock::now();        
    auto nextAccel = last + ACCEL_MAX_PERIOD;
    auto nextAvr = last + AVR_MAX_PERIOD;
    std::unique_lock g{m_};
    while (true) {
        // while there are any events in the queue, proces them while we are under lock
        while (!events_.empty()) {
            Event ev = events_.front();
            last = std::chrono::system_clock::now();
            events_.pop_front();
            g.unlock();
            switch (ev) {
                case Event::AvrIrq:
                    if (last >= nextAvr) {
                        queryAvrFull();
                        nextAvr = last + AVR_MAX_PERIOD;
                    } else {
                        queryAvrStatus();
                    };
                    break;
                case Event::RadioIrq:
                    queryRadio();
                    break;
                // the power off sequence
                case Event::PowerOff:
                    // TODO 
                    break;
            }
            g.lock();
        }
        cv_.wait_until(g, nextAccel, [this](){ return ! events_.empty(); }); 
        last = std::chrono::system_clock::now();
        if (last >= nextAccel) {
            g.unlock();
            queryAccel();
            nextAccel = last + ACCEL_MAX_PERIOD;
            g.lock();
        }
        if (last >= nextAvr) 
            events_.push_back(Event::AvrIrq); // don't call emit event since we already have lock
    }
}

void Driver::queryAvrStatus() {
    comms::Status state;
    i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
    //std::cout << (int)state.status_ << " " << (int)state.joyX_ << " " << (int)state.joyY_ << " " << (int)state.photores_ << std::endl;
    updateStatus(state);
}

void Driver::queryAvrFull() {
    comms::FullState state;
    i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
    // update the basic status
    updateStatus(state.status);
    // update the extended state - first get lock on the data
    //LOG("VBatt: " << state.estatus.vbatt());
    //LOG("VCC: " << state.estatus.vcc());
    //LOG("Temp: " << state.estatus.temp());
    mState_.lock();
    bool vbattChanged = batteryVoltage_ != state.estatus.vbatt();
    batteryVoltage_ = state.estatus.vbatt();
    bool tempChanged = tempAvr_ != state.estatus.temp();
    tempAvr_ = state.estatus.temp();
    mState_.unlock();
    // emit events where necessary
    if (vbattChanged)
        emit batteryVoltageChanged(batteryVoltage_);
    if (tempChanged)
        emit tempAvrChanged(tempAvr_); 
}

void Driver::updateStatus(comms::Status status) {
    // get the lock on the driver's state and perform updates based on the status, remembering the events that need to be raised later on when the lock is released
    mState_.lock();
    bool volLeftChanged = volumeLeft_.update(status.btnVolumeLeft());
    bool volRightChanged = volumeRight_.update(status.btnVolumeRight());
    bool thumbChanged = thumbBtn_.update(status.btnJoystick());
    bool thumbPosChanged = thumbX_.update(status.joyX());
    thumbPosChanged = thumbY_.update(status.joyY()) || thumbPosChanged;

    bool chrgChanged = charging_ != status.charging();
    charging_ = status.charging();
    bool lowBattChanged = lowBattery_ != status.lowBattery();
    lowBattery_ = status.lowBattery();

    // release the lock and emit the necessary events
    mState_.unlock();
    if (volLeftChanged)
        emit buttonVolumeLeft(status.btnVolumeLeft());
    if (volRightChanged)
        emit buttonVolumeRight(status.btnVolumeRight());
    if (thumbChanged)
        emit buttonVolumeLeft(status.btnJoystick());

    if (thumbPosChanged)
        emit thumbstick(status.joyX(), status.joyY());

    if (chrgChanged)
        emit chargingChanged(charging_);
    if (lowBattChanged)
        emit lowBatteryChanged();

}

void Driver::queryAccel() {
    MPU6050::AccelData d = accel_.readAccel();
    // also read the temperature so that we have one more datapoint whether the handheld overheats or not
    uint16_t t = accel_.readTemp();
    d.x = accelTo1GUnsigned(- d.x);
    d.y = accelTo1GUnsigned(- d.y);
    mState_.lock();
    bool accelChanged = accelX_.update(d.x);
    accelChanged = accelY_.update(d.y) || accelChanged;
    // determine if we should emit the dpad events
    if (accelChanged && dpadState_ == DPadState::Accel) {
        updateDPad(d.x, d.y);
    } else {
        mState_.unlock();
        if (accelChanged)
            emit accel(d.x, d.y);
    }
}

uint8_t Driver::accelTo1GUnsigned(int16_t v) {
    if (v < -16384)
        v = -16384;
    if (v >= 16384)
        v = 16383;
    v += 16384;
    return (v >> 7);    
}


void Driver::queryRadio() {

}

void Driver::isrButtonChange(int gpio, int level, uint32_t tick, Button * btn) {
    bool state = ! level;
    {
        std::lock_guard g_{singleton_->mState_};
        if (! btn->update(state))
            return;
    }
    // emit corresponding qt signal
    switch (gpio) {
        case PIN_BTN_A:
            emit singleton_->buttonA(state);
            break;
        case PIN_BTN_B:
            emit singleton_->buttonB(state);
            break;
        case PIN_BTN_X:
            emit singleton_->buttonX(state);
            break;
        case PIN_BTN_Y:
            emit singleton_->buttonY(state);
            break;
        case PIN_BTN_L:
            emit singleton_->buttonLeft(state);
            break;
        case PIN_BTN_R:
            emit singleton_->buttonRight(state);
            break;
        case PIN_BTN_SELECT:
            emit singleton_->buttonSelect(state);
            break;
        case PIN_BTN_START:
            emit singleton_->buttonStart(state);
            break;
    }
    if (btn->state == state)
        return;
}

void Driver::isrHeadphonesChange(int gpio, int level, uint32_t tick, Button * btn) {
    {
        std::lock_guard g_{singleton_->mState_};
        if (singleton_->headphones_ == level)
            return;
        singleton_->headphones_ = level;
    }
    emit singleton_->headphonesChanged(level);        
}

void Driver::initializeLibevdevDevice() {
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

        libevdev_enable_event_code(dev, EV_KEY, dpadUp_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, dpadDown_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, dpadLeft_.evdevId, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, dpadRight_.evdevId, nullptr);

        int err = libevdev_uinput_create_from_device(dev,
                                                LIBEVDEV_UINPUT_OPEN_MANAGED,
                                                &uidev_);
        // TODO where to report this error???? ANd how toreport errors in general, restart avr?
        if (err != 0)
            std::cout << "cannot do what I want to do: " << err << std::endl;
        libevdev_free(dev);
}

void Driver::initializePins() {
    gpio::inputPullup(PIN_BTN_A);
    gpio::inputPullup(PIN_BTN_B);
    gpio::inputPullup(PIN_BTN_X);
    gpio::inputPullup(PIN_BTN_Y);
    gpio::inputPullup(PIN_BTN_L);
    gpio::inputPullup(PIN_BTN_R);
    gpio::inputPullup(PIN_BTN_SELECT);
    gpio::inputPullup(PIN_BTN_START);
    gpio::inputPullup(PIN_AVR_IRQ);
    gpio::input(PIN_HEADPHONES);
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
#endif

}

void Driver::updateDPad(uint8_t horizontal, uint8_t vertical) {
    TriState changedUp = dpadUp_.update(vertical);
    TriState changedDown = dpadDown_.update(vertical);
    TriState changedLeft = dpadLeft_.update(horizontal);
    TriState changedRight = dpadRight_.update(horizontal);
    mState_.unlock();
    if (changedUp != TriState::Unchanged)
        emit dpadUp(changedUp == TriState::On);
    if (changedDown != TriState::Unchanged)
        emit dpadDown(changedDown == TriState::On);
    if (changedLeft != TriState::Unchanged)
        emit dpadLeft(changedLeft == TriState::On);
    if (changedRight != TriState::Unchanged)
        emit dpadRight(changedRight == TriState::On);

}

