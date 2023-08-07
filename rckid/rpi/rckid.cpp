#include <iostream>

#include "../avr-i2c-bootloader/src/programmer.h"

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

RCKid & RCKid::create() {
    auto & i = instance();
    ASSERT(i.get() == nullptr && "RCKid must be a singleton");
    i = std::unique_ptr<RCKid>(new RCKid{});
    return *i.get();
}

RCKid::RCKid() {
    gpio::initialize();
    if (!spi::initialize()) 
        TraceLog(LOG_ERROR, STR("Unable to initialize spi (errno " << errno << ")"));
    if (!i2c::initializeMaster())
        TraceLog(LOG_ERROR, STR("Unable to initialize i2c (errno " << errno << "), make sure /dev/i2c1 exists"));
    initializeLibevdev();
    initializeAvr();
    initializeAccel();
    initializeNrf();
    initializeISRs();

    // start the hw loop thread
    tHwLoop_ = std::thread{[this](){
        // get the extended state first and initialize 
        // TODO the below is wrong
        {
            comms::ExtendedState state;
            i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
            processAvrExtendedState(state);
        }
        setBrightness(255);
        while (!shouldTerminate_.load()) {
            DriverEvent e = driverEvents_.waitReceive();
            processDriverEvent(e);
        }
    }};
    tTicks_ = std::thread{[this](){
        while (!shouldTerminate_.load()) {
            cpu::delayMs(10);
            this->driverEvents_.send(Tick{});
        }
    }};
    tSeconds_ = std::thread{[this](){
        while (!shouldTerminate_.load()) {
            cpu::delayMs(1000);
            this->driverEvents_.send(SecondTick{});
        }
    }};

#if (defined ARCH_MOCK)
    std::thread mockRecording{[this](){
        std::vector<uint8_t> rec;
        std::ifstream f{"assets/recording/test.dat"};
        while (!f.eof())
            rec.push_back(f.get());
        TraceLog(LOG_DEBUG, STR("Loaded mock recording with size " << rec.size()).c_str());
        size_t i = 0;
        while (true) {
            cpu::delayMs(4);
            if (mockRecording_) {
                RecordingEvent e;
                e.status.setRecording(true);
                e.status.setBatchIndex(mockRecBatch_);
                mockRecBatch_ = (mockRecBatch_ + 1) % 8;
                for (size_t x = 0; x < 32; ++x) {
                    e.data[x] = rec[i];
                    i = (i + 1) % rec.size();
                }
                uiEvents_.send(e);
            }
        }
    }};
    mockRecording.detach();
#endif
}

std::optional<Event> RCKid::nextEvent() {
    auto e = uiEvents_.receive();
    // TODO process events we are interested in for statistics
    return e;
}


void RCKid::processDriverEvent(DriverEvent e) {
    std::visit(overloaded{
        // do nothing for termination, it's sent just to ensure the thread will wake up and can react to shouldTerminate flag
        [this](Terminate) {},
        [this](Tick){
#if (defined ARCH_MOCK)        
            checkMockButtons();
#endif
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
            // only query the accell and photores status when we are not recording to keep the I2C fully for the audio recorder
            if (!state_.status.recording()) {
                queryAccelStatus();
                // TODO query photores
            }
        },
        [this](SecondTick) {
            // only query extended state if we are not recording audio at the same time
            if (!state_.status.recording())
                queryAvrExtendedState();
        },
        // this could be either input interrupt, or recording interrupt. If we are not aware in the status that recording has started yet, try the input reading, which also updates the status, and if this update switches to recording, abort the input and go to recording instead. 
        [this](AvrIrq) {
            if (!state_.status.recording()) {
                comms::State state{queryAvrState()};
                {
                    std::lock_guard<std::mutex> g{mState_};
                    processAvrStatus(state.status, true);
                    if (!state_.status.recording()) {
                        processAvrControls(state.controls, true);
                        return;
                    }
                }
            }
            getAvrRecording();
        }, 
        [this](HeadphonesIrq e) {
            { 
                std::lock_guard<std::mutex> g{mState_};
                headphones_ = platform::gpio::read(PIN_HEADPHONES);
            }
            uiEvents_.send(HeadphonesEvent{headphones_});
        },
        [this](ButtonIrq e) {
            if (e.btn.update(e.state))
                buttonAction(e.btn);
        },
        // keyboard presses
        [this](KeyPress e) {
            if (gamepad_ != nullptr) {
                libevdev_uinput_write_event(gamepad_, EV_KEY, e.key, e.state);
                libevdev_uinput_write_event(gamepad_, EV_SYN, SYN_REPORT, 0);
            } else {
                TraceLog(LOG_WARNING, "Cannot emit key - keyboard not available");
            }
        },
        [this](NRFIrq) {
            // if not in transmit mode, the IRQ must be received packet, read any received packets and send them to the UI 
            if (!nrfTx_) {
                NRFPacketEvent e;
                nrf_.clearDataReadyIrq();
                while (nrf_.receive(e.packet, 32))
                    uiEvents_.send(e);
            // otherwise the irq was a tx irq, which means a message has been transmitted "successfully" - since we don't use ACKs due to NRF modules incompatibility we actually don't know this for sure and any message send will always succeed. 
            } else {
                nrf_.clearIrq();
                uiEvents_.send(NRFTxEvent{});
                // we are now in standby 1 mode (assuming we transmit one message per invocation). Check if there is more messages, and if yes, trasnmit them immediately w/o going to real standby
                mRadio_.lock();
                if (! nrfTxQueue_.empty()) {
                    NRFPacket p{nrfTxQueue_.front()};
                    nrfTxQueue_.pop_front();
                    mRadio_.unlock();
                    nrf_.transmit(p.packet, 32); // since in standby-1, will be transmitted immediately
                // if no more messages, go to either standby (if Tx) or enable receiver if this was a tx burst from rx mode
                } else {
                    mRadio_.unlock();
                    if (nrfState_ == NRFState::Rx)
                        nrf_.enableReceiver();
                    else
                        nrf_.standby();
                    nrfTx_ = false;
                }
            }
        }, 
        [this](NRFInitialize e) {
            // TODO process error
            nrf_.initialize(e.rxAddr, e.txAddr, e.channel);
            nrf_.standby();
            std::lock_guard<std::mutex> g{mRadio_};
            nrfState_ = NRFState::Standby;
        },
        [this](NRFState e) {
            switch (e) {
                case NRFState::Standby:
                    nrf_.standby();
                    break;
                case NRFState::PowerDown:
                    nrf_.powerDown();
                    break;
                case NRFState::Rx:
                    nrf_.enableReceiver();
                    break;
                case NRFState::Tx:
                    nrf_.standby();
                    break;
            }
            nrfTx_ = false;
            std::lock_guard<std::mutex> g{mRadio_};
            nrfState_ = e;
        }, 
        [this](NRFTransmit e) {
            nrfTx_ = true;
            mRadio_.lock();
            NRFPacket p{nrfTxQueue_.front()};
            nrfTxQueue_.pop_front();
            mRadio_.unlock();
            nrf_.transmit(p.packet, 32);
            nrf_.enableTransmitter();
        },
        [this](NRFPacket e) {
            nrf_.transmit(e.packet, 32);
            nrf_.enablePolledTransmitter();
            NRF24L01::Status status = nrf_.clearTxIrqs();
            while (!status.txDataSentIrq() && !status.txDataFailIrq())
                status = nrf_.clearTxIrqs();
            nrf_.enableReceiver();
            // send UI confirmation
            uiEvents_.send(NRFTxEvent{});
            // if there is tx irq, process it (means we have received a message while transmitting and enabling the receiver
            if (status.rxDataReady()) {
                NRFPacketEvent e;
                nrf_.clearDataReadyIrq();
                while (nrf_.receive(e.packet, 32))
                    uiEvents_.send(e);
            }
        },
        [this](auto msg) {
            sendAvrCommand(msg);
        }
    }, e);
}


void RCKid::initializeAvr() {
    // check if the AVR is present at all
    if (!i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, nullptr, 0))
        TraceLog(LOG_ERROR, STR("AVR not found:" << errno));
    // check if the AVR is in bootloader mode
    try {
        Programmer pgm{AVR_I2C_ADDRESS, RPI_PIN_AVR_IRQ};
        pgm.setLogLevel(Programmer::LOG_TRACE);
        while (true) {
            ChipInfo ci{pgm.getChipInfo()};
            if (ci.state.status != bootloader::BOOTLOADER_MODE)
                break;
            TraceLog(LOG_WARNING, "AVR in bootloader mode");
            pgm.resetToApp();
            cpu::delayMs(500);
        }
    } catch (std::exception const & e) {
        TraceLog(LOG_ERROR, STR("Cannot talk to AVR: " << e.what()));
    }
    // we are now in non-bootloader mode, check all is good
    comms::Status status = queryAvrStatus();
    TraceLog(LOG_INFO, STR("Power-on AVR status: " << status).c_str());
    // if we are to terminate immediately because the current status is power down, shutdown and terminate itself immediately
    if (status.mode() == comms::Mode::PowerDown) {
        TraceLog(LOG_INFO, "Spurious power-up. Powering down immediately");
        system("sudo poweroff");
        exit(EXIT_SUCCESS);
    }
    // attach the interrupt
    gpio::inputPullup(PIN_AVR_IRQ);
    gpio::attachInterrupt(PIN_AVR_IRQ, gpio::Edge::Falling, & isrAvrIrq);
}

void RCKid::processAvrStatus(comms::Status status, bool alreadyLocked) {
    utils::cond_lock_guard g{mState_, alreadyLocked};
    if (state_.status.mode() != status.mode()) {
        switch (state_.status.mode()) {
            case comms::Mode::PowerDown: 
                TraceLog(LOG_INFO, "Power down requested");
                system("sudo poweroff");
                break;
            case comms::Mode::WakeUp:
                TraceLog(LOG_WARNING, "WakeUp mode detected when powering on");
                // fallthrough
            case comms::Mode::PowerUp:
                TraceLog(LOG_INFO, "PowerUp mode, switching to ON");
                // enter the power-on mode to disable any timeouts
                sendAvrCommand(msg::PowerOn{});
        }
        state_.status.setMode(status.mode());
        uiEvents_.send(status.mode());
    }
    if (state_.status.recording() != status.recording()) {
        state_.status.setRecording(status.recording());
    }
    if (state_.status.alarm() != status.alarm()) {
        state_.status.setAlarm(status.alarm());
        uiEvents_.send(AlarmEvent{});
    }
    if (state_.status.lowBattery() != status.lowBattery()) {
        state_.status.setLowBattery(status.lowBattery());
        uiEvents_.send(LowBatteryEvent{});
    }
}

void RCKid::processAvrControls(comms::Controls controls, bool alreadyLocked) {
    utils::cond_lock_guard g{mState_, alreadyLocked};
    if (state_.controls.select() != controls.select()) {
        state_.controls.setSelect(controls.select());
        uiEvents_.send(ButtonEvent{Button::Select, controls.select()});
    }
    if (state_.controls.start() != controls.start()) {
        state_.controls.setStart(controls.start());
        uiEvents_.send(ButtonEvent{Button::Start, controls.start()});
    }
    if (state_.controls.home() != controls.home()) {
        state_.controls.setButtonHome(controls.home());
        uiEvents_.send(ButtonEvent{Button::Home, controls.home()});
    }

    // TODO remove this and replace with L & R buttons in new revision
    if (state_.controls.dpadUp() != controls.dpadUp()) {
        state_.controls.setDpadUp(controls.dpadUp());
        uiEvents_.send(ButtonEvent{Button::Up, controls.dpadUp()});
    }
    if (state_.controls.dpadDown() != controls.dpadDown()) {
        state_.controls.setDpadDown(controls.dpadDown());
        uiEvents_.send(ButtonEvent{Button::Down, controls.dpadDown()});
    }
    if (state_.controls.dpadLeft() != controls.dpadLeft()) {
        state_.controls.setDpadLeft(controls.dpadLeft());
        uiEvents_.send(ButtonEvent{Button::Left, controls.dpadLeft()});
    }
    if (state_.controls.dpadRight() != controls.dpadRight()) {
        state_.controls.setDpadRight(controls.dpadRight());
        uiEvents_.send(ButtonEvent{Button::Right, controls.dpadRight()});
    }
   
    // joystick reading
    bool report = false;
    if (joyX_.update(controls.joyH())) {
        state_.controls.setJoyH(controls.joyH());
        axisAction(joyX_, true);
        report = true;
    }
    if (joyY_.update(controls.joyV())) {
        state_.controls.setJoyV(controls.joyV());
        axisAction(joyY_, true);
        report = true;
    }
    if (report)
        uiEvents_.send(JoyEvent{joyX_.reportedValue, joyY_.reportedValue});
}

void RCKid::processAvrExtendedState(comms::ExtendedState & state) {
    std::lock_guard g{mState_};
    processAvrStatus(state.status, true);
    processAvrControls(state.controls, true);
    bool report = false;
    if (state_.einfo.vcc() != state.einfo.vcc()) {
        report = true;
        state_.einfo.setVcc(state.einfo.vcc());
    }   
    if (state_.einfo.vBatt() != state.einfo.vBatt()) {
        report = true;
        state_.einfo.setVBatt(state.einfo.vBatt());
    }
    if (state_.einfo.temp() != state.einfo.temp()) {
        report = true;
        state_.einfo.setTemp(state.einfo.temp());
    }
    if (state_.einfo.brightness() != state.einfo.brightness()) {
        report = true;
        state_.einfo.setBrightness(state.einfo.brightness());
    } 
    // TODO process the rest 
    if (report)
        uiEvents_.send(StateChangeEvent{});
}



void RCKid::initializeAccel() {
    if (accel_.deviceIdentification() == 104) {
        accel_.reset();
    } else {
        TraceLog(LOG_ERROR, "Accel not found");
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

void RCKid::queryAccelStatus() {
    MPU6050::AccelData d = accel_.readAccel();
    int16_t t = accel_.readTemp();
    uint8_t x = accelTo1GUnsigned(-d.x);
    uint8_t y = accelTo1GUnsigned(-d.y);
    bool report = false;
    if (accelX_.update(x)) {
        report = true;
        axisAction(accelX_);
    }
    if (accelY_.update(y)) {
        report = true;
        axisAction(accelY_);
    }
    if (accelTemp_ != t) {
        std::lock_guard<std::mutex> g{mState_};
        accelTemp_ = t;
    }
    if (report)
        uiEvents_.send(AccelEvent{accelX_.reportedValue, accelY_.reportedValue});
}

void RCKid::initializeNrf() {
    if (nrf_.initialize("RCKID", "RCKID")) {
        nrf_.standby();
        nrfState_ = NRFState::Standby;
    } else {
        TraceLog(LOG_ERROR, "Radio not found");
        nrfState_ = NRFState::Error;
    }
    // attach the interrupt handler
    gpio::input(PIN_NRF_IRQ);
    gpio::attachInterrupt(PIN_NRF_IRQ, gpio::Edge::Falling, & isrNrfIrq);
}

void RCKid::initializeISRs() {
    gpio::input(PIN_HEADPHONES);
    gpio::attachInterrupt(PIN_HEADPHONES, gpio::Edge::Both, & isrHeadphones);

    gpio::inputPullup(PIN_BTN_A);
    gpio::inputPullup(PIN_BTN_B);
    gpio::inputPullup(PIN_BTN_X);
    gpio::inputPullup(PIN_BTN_Y);
    gpio::inputPullup(PIN_BTN_DPAD_UP);
    gpio::inputPullup(PIN_BTN_DPAD_DOWN);
    gpio::inputPullup(PIN_BTN_DPAD_LEFT);
    gpio::inputPullup(PIN_BTN_DPAD_RIGHT);
    gpio::inputPullup(PIN_BTN_JOY);
    gpio::attachInterrupt(PIN_BTN_A, gpio::Edge::Both, & isrButtonA);
    gpio::attachInterrupt(PIN_BTN_B, gpio::Edge::Both, & isrButtonB);
    gpio::attachInterrupt(PIN_BTN_X, gpio::Edge::Both, & isrButtonX);
    gpio::attachInterrupt(PIN_BTN_Y, gpio::Edge::Both, & isrButtonY);
    /* TODO re-enable for new revision
    gpio::attachInterrupt(PIN_BTN_DPAD_UP, gpio::Edge::Both, & isrButtonDpadUp);
    gpio::attachInterrupt(PIN_BTN_DPAD_DOWN, gpio::Edge::Both, & isrButtonDpadDown);
    gpio::attachInterrupt(PIN_BTN_DPAD_LEFT, gpio::Edge::Both, & isrButtonDpadLeft);
    gpio::attachInterrupt(PIN_BTN_DPAD_RIGHT, gpio::Edge::Both, & isrButtonDpadRight);
    */
    gpio::attachInterrupt(PIN_BTN_JOY, gpio::Edge::Both, & isrButtonJoy);
}

void RCKid::initializeLibevdev() {
    gamepadDev_ = libevdev_new();
    libevdev_set_name(gamepadDev_, LIBEVDEV_GAMEPAD_NAME);
    libevdev_set_id_bustype(gamepadDev_, BUS_USB);
    libevdev_set_id_vendor(gamepadDev_, 0x0ada);
    libevdev_set_id_product(gamepadDev_, 0xbabe);
    // enable keys for the buttons and the axes (dpad, thumb, accel)
    libevdev_enable_event_type(gamepadDev_, EV_KEY);
    libevdev_enable_event_type(gamepadDev_, EV_ABS);

    libevdev_enable_event_code(gamepadDev_, EV_KEY, RETROARCH_HOTKEY_ENABLE, nullptr);

    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnA_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnB_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnX_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnY_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnL_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnR_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnSelect_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnStart_.evdevId, nullptr);
    // dpad
    input_absinfo thumb{
        .value = 0,
        .minimum = -1,
        .maximum = 1,
        .fuzz = 0,
        .flat = 0,
        .resolution = 1
    };

    libevdev_enable_event_code(gamepadDev_, EV_ABS, btnDpadUp_.evdevId, &thumb);
    libevdev_enable_event_code(gamepadDev_, EV_ABS, btnDpadLeft_.evdevId, &thumb);
    // thumbstick button
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnJoy_.evdevId, nullptr);
    // enable the thumbstick and accelerometer
    input_absinfo info {
        .value = 128,
        .minimum = 0,
        .maximum = 255,
        .fuzz = 0,
        .flat = 0,
        .resolution = 1,
    };
    libevdev_enable_event_code(gamepadDev_, EV_ABS, joyX_.evdevId, & info);
    libevdev_enable_event_code(gamepadDev_, EV_ABS, joyY_.evdevId, & info);
    libevdev_enable_event_code(gamepadDev_, EV_ABS, accelX_.evdevId, & info);
    libevdev_enable_event_code(gamepadDev_, EV_ABS, accelY_.evdevId, & info);


    int err = libevdev_uinput_create_from_device(gamepadDev_,
                                            LIBEVDEV_UINPUT_OPEN_MANAGED,
                                            &gamepad_);
    if (err != 0) 
        TraceLog(LOG_ERROR, STR("Unable to setup gamepad (result " << err << ", errno: " << errno << ")"));
}

#if (defined ARCH_MOCK)
void RCKid::checkMockButtons() {
    // need to poll the events since we may have 0 framerate when updates to the screen are not necessary
    PollInputEvents();
#define CHECK_KEY(KEY, BTN) if (IsKeyDown(KEY) != BTN.actualState) if (BTN.update(! BTN.actualState)) buttonAction(BTN);
    CHECK_KEY(KeyboardKey::KEY_A, btnA_);
    CHECK_KEY(KeyboardKey::KEY_B, btnB_);
    CHECK_KEY(KeyboardKey::KEY_X, btnX_);
    CHECK_KEY(KeyboardKey::KEY_Y, btnY_);
    CHECK_KEY(KeyboardKey::KEY_L, btnL_);
    CHECK_KEY(KeyboardKey::KEY_R, btnR_);
    CHECK_KEY(KeyboardKey::KEY_D, btnJoy_);
    CHECK_KEY(KeyboardKey::KEY_ENTER, btnSelect_);
    CHECK_KEY(KeyboardKey::KEY_SPACE, btnStart_);
    CHECK_KEY(KeyboardKey::KEY_LEFT, btnDpadLeft_);
    CHECK_KEY(KeyboardKey::KEY_RIGHT, btnDpadRight_);
    CHECK_KEY(KeyboardKey::KEY_UP, btnDpadUp_);
    CHECK_KEY(KeyboardKey::KEY_DOWN, btnDpadDown_);
    CHECK_KEY(KeyboardKey::KEY_H, btnHome_);
#undef CHECK_KEY
}
#endif


#ifdef HAHA

// ----------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------

RCKid & RCKid::create() {
    singleton_ = std::unique_ptr<RCKid>{new RCKid{}};
    return *singleton_;
}


RCKid::RCKid() {
    gpio::initialize();
    if (!spi::initialize()) 
        TraceLog(LOG_ERROR, STR("Unable to initialize spi (errno " << errno << ")"));
    if (!i2c::initializeMaster())
        TraceLog(LOG_ERROR, STR("Unable to initialize i2c (errno " << errno << "), make sure /dev/i2c1 exists"));
    RCKid * & i = instance();
    ASSERT(i == nullptr && "RCKid must be a singleton");
    i = this;
    initializeLibevdevGamepad();
    initializeAvr();
    initializeAccel();
    initializeNrf();
    initializeISRs();

    // start the hw loop thread
    tHwLoop_ = std::thread{[this](){
        this->hwLoop();
    }};
    //t.detach();
    tTicks_ = std::thread{[this](){
        while (!shouldTerminate_.load()) {
            cpu::delayMs(10);
            this->hwEvents_.send(Tick{});
        }
    }};
    tSeconds_ = std::thread{[this](){
        while (!shouldTerminate_.load()) {
            cpu::delayMs(1000);
            this->hwEvents_.send(SecondTick{});
        }
    }};

#if (defined ARCH_MOCK)
    std::thread mockRecording{[this](){
        std::vector<uint8_t> rec;
        std::ifstream f{"assets/recording/test.dat"};
        while (!f.eof())
            rec.push_back(f.get());
        TraceLog(LOG_DEBUG, STR("Loaded mock recording with size " << rec.size()).c_str());
        size_t i = 0;
        while (true) {
            cpu::delayMs(4);
            if (mockRecording_) {
                RecordingEvent e;
                e.status.setRecording(true);
                e.status.setBatchIndex(mockRecBatch_);
                mockRecBatch_ = (mockRecBatch_ + 1) % 8;
                for (size_t x = 0; x < 32; ++x) {
                    e.data[x] = rec[i];
                    i = (i + 1) % rec.size();
                }
                events_.send(e);
            }
        }
    }};
    mockRecording.detach();
#endif
}

std::optional<Event> RCKid::nextEvent() {
    auto e = events_.receive();
    if (e.has_value()) {
        std::visit(overloaded{
            [this](AccelEvent ea) {
                //if (setIfDiffers(status_.accelTemp, ea.temp))
                //    {} // TODO
            }, 
            [this](ModeEvent & e) {
                status_.changed = true;
                status_.mode = e.mode;
            },
            [this](ChargingEvent & e) {
                status_.changed = true;
                status_.usb = e.usb;
                status_.charging = e.charging;
            },
            [this](VoltageEvent & e) {
                status_.changed = true;
                status_.vBatt = e.vBatt;
                status_.vcc = e.vcc;
            },
            [this](TempEvent & e) {
                status_.changed = true;
                status_.avrTemp = e.temp;
            },
            [this](BrightnessEvent & e) {
                status_.changed = true;
                status_.brightness = e.brightness;
            },
            [this](HeadphonesEvent & e) {
                status_.changed = true;
                status_.headphones = e.connected;
            },
            [this](UptimeEvent & e) {
                status_.avrUptime = e.uptime;
            },
            [this](NRFTxIrq & e) {
                nrfState_ = e.newState;
                nrfTxQueueSize_ = e.txQueueSize;
            },
            // don't do anything for the others
            [this](auto & e) {}
        }, e.value());
    }
    return e;
}

void RCKid::hwLoop() {
    // query avr status
    avrQueryExtendedState();
    setBrightness(255);
    bool terminate = false;
    while (! terminate) {
        std::visit(overloaded{
            [&terminate](Terminate) { 
                terminate = true; 
            },
            [this](Tick){
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
                // only query the accell and photores status when we are not recording to keep the I2C fully for the audio recorder
                if (!driverStatus_.recording) {
                    accelQueryStatus();
                    // TODO query photores
                }
            },
            [this](SecondTick) {
                // only query extended state if we are not recording audio at the same time
                if (!driverStatus_.recording)
                    avrQueryExtendedState();
            },
            [this](Irq irq) {
                switch (irq.pin) {
                    case PIN_AVR_IRQ:
                        if (driverStatus_.recording)
                            avrGetRecording();
                        else
                            avrQueryState();
                        break;
                    case PIN_NRF_IRQ:
                        if (driverStatus_.nrfState == NRFState::Transmitting)
                            nrfTxDone();
                        else
                            nrfReceivePackets();
                        break;
                    default: // don't do anything for irq's we do not care about
                        break;
                }
            },
            // keyboard presses
            [this](KeyPress e) {
                if (gamepad_ != nullptr) {
                    libevdev_uinput_write_event(gamepad_, EV_KEY, e.key, e.state);
                    libevdev_uinput_write_event(gamepad_, EV_SYN, SYN_REPORT, 0);
                } else {
                    TraceLog(LOG_WARNING, "Cannot emit key - keyboard not available");
                }
            },
            [this](EnableGamepad e) {
                activeDevice_ = e.enable ? gamepad_ : nullptr;
            },
            [this](NRFInitialize e) {
                // TODO process error
                nrf_.initialize(e.rxAddr, e.txAddr, e.channel);
                nrf_.standby();
                driverStatus_.nrfState = NRFState::Standby;
            },
            [this](NRFStateChange e) {
                switch (e.state) {
                    case NRFState::Standby:
                        nrf_.standby();
                        break;
                    case NRFState::PowerDown:
                        nrf_.powerDown();
                        break;
                    case NRFState::Receiver:
                        nrf_.enableReceiver();
                        break;
                }
                driverStatus_.nrfState = e.state;
            },
            [this](NRFTransmit e) {
                if (e.immediateReturn && driverStatus_.nrfState == NRFState::Receiver) {
                    nrf_.transmit(e.packet, 32);
                    nrf_.enablePolledTransmitter();
                    NRF24L01::Status status = nrf_.clearTxIrqs();
                    while (!status.txDataSentIrq() && !status.txDataFailIrq())
                        status = nrf_.clearTxIrqs();
                    nrf_.enableReceiver();
                    events_.send(NRFTxIrq{status, driverStatus_.nrfState, driverStatus_.nrfTxQueue.size()});
                } else if (driverStatus_.nrfState != NRFState::Transmitting) {
                    driverStatus_.nrfReceiveAfterTransmit = driverStatus_.nrfState == NRFState::Receiver;
                    nrf_.standby();
                    driverStatus_.nrfState = NRFState::Transmitting;
                    nrf_.transmit(e.packet, 32);
                    nrf_.enableTransmitter();
                } else {
                    driverStatus_.nrfTxQueue.push_back(e);
                }
            },
            [this](msg::StartAudioRecording e) {
                sendAvrCommand(e);
                driverStatus_.recording = true;
            },
            [this](msg::StopAudioRecording e) {
                sendAvrCommand(e);
                driverStatus_.recording = false;
            },
            [this](auto msg) {
                sendAvrCommand(msg);
            }
        }, hwEvents_.waitReceive());
    }
}

#if (defined ARCH_MOCK)
void RCKid::checkMockButtons() {
    // need to poll the events since we may have 0 framerate when updates to the screen are not necessary
    PollInputEvents();
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

void RCKid::accelQueryStatus() {
    MPU6050::AccelData d = accel_.readAccel();
    int16_t t = accel_.readTemp();
    uint8_t x = accelTo1GUnsigned(-d.x);
    uint8_t y = accelTo1GUnsigned(-d.y);
    bool report = axisChange(accelTo1GUnsigned(-d.x), accelX_);
    report = axisChange(accelTo1GUnsigned(-d.y), accelY_) || report;
    // TODO convert the temperature and update it as well
    //if (report)
    //    events_.send(AccelEvent{accelX_.current, accelY_.current, accelTo1GUnsigned(-d.z), t});
}

comms::Status RCKid::avrQueryStatus() {
    comms::Status status;
    i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& status, sizeof(status));
    return status;
}

void RCKid::avrQueryState() {
    comms::State state;
    i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
    processAvrStatus(state.status);
    if (!state.status.recording())
        processAvrControls(state.controls);
}

void RCKid::avrQueryExtendedState() {
    comms::ExtendedState state;
    i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state, sizeof(state));
    processAvrStatus(state.status);
    if (! state.status.recording()) {
        processAvrControls(state.controls);
        processAvrExtendedInfo(state.einfo);
        // if its the first time boot, clear the dinfo error
        if (state.dinfo.errorCode() == comms::ErrorCode::InitialPowerOn) {
            TraceLog(LOG_WARNING, "First boot detected");
            msg::DInfoClear m;
            i2c::transmit(AVR_I2C_ADDRESS, (uint8_t*)&m, sizeof(m), nullptr, 0);
        }
        if (driverStatus_.avrUptime != state.uptime) {
            driverStatus_.avrUptime = state.uptime;
            events_.send(UptimeEvent{state.uptime});
        }
    }
}

void RCKid::processAvrStatus(comms::Status const & status) {
    if (! status.recording()) {
        switch (status.mode()) {
            case comms::Mode::PowerDown: 
                TraceLog(LOG_INFO, "Power down requested");
                system("sudo poweroff");
                break;
            case comms::Mode::WakeUp:
                TraceLog(LOG_WARNING, "WakeUp mode detected when powering on");
                // fallthrough
            case comms::Mode::PowerUp:
                TraceLog(LOG_INFO, "PowerUp mode, switching to ON");
                // enter the power-on mode to disable any timeouts
                sendAvrCommand(msg::PowerOn{});
        }
        if (setIfDiffers(mode_.mode, status.mode()))
            events_.send(mode_);
    }
}

void RCKid::processAvrControls(comms::Controls const & controls) {
#if (defined ARCH_MOCK)        
    // don't process the controls in mock mode
    return;
#endif
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

    bool report = axisChange(controls.joyH(), thumbX_);
    report = axisChange(controls.joyV(), thumbY_) || report;
    if (report)
      events_.send(ThumbEvent{thumbX_.current, thumbY_.current});
}

void RCKid::processAvrExtendedInfo(comms::ExtendedInfo const & einfo) {
    if (setIfDiffers(voltage_, VoltageEvent{einfo.vBatt(), einfo.vcc()}))
        events_.send(voltage_);
    if (setIfDiffers(temp_, TempEvent{einfo.temp()}))
        events_.send(temp_);
    if (setIfDiffers(brightness_, BrightnessEvent{einfo.brightness()}))
        events_.send(brightness_);
    if (setIfDiffers(charging_, ChargingEvent{einfo.usb(), einfo.charging()}))
        events_.send(charging_);
   
    // TODO other einfo members
}

void RCKid::avrGetRecording() {
    RecordingEvent r;
    i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)(&r), sizeof(RecordingEvent));
    // do the normal status processing as we would in non-recording mode
    processAvrStatus(r.status);
    if (r.status.recording() && !r.status.batchIncomplete())
        events_.send(r);
}

void RCKid::nrfReceivePackets() {
    NRFPacketEvent e;
    nrf_.clearDataReadyIrq();
    while (nrf_.receive(e.packet, 32))
        events_.send(e);
}

void RCKid::nrfTxDone() {
    NRF24L01::Status status = nrf_.getStatus();
    nrf_.clearIrq();
    // we are now in standby 1 mode (assuming we transmit one message per invocation). Check if there is more messages, and if yes, trasnmit them immediately w/o going to real standby
    if (! driverStatus_.nrfTxQueue.empty()) {
        nrf_.transmit(driverStatus_.nrfTxQueue.front().packet, 32);
        driverStatus_.nrfTxQueue.pop_front();
        // send the IRQ events with current state 
        events_.send(NRFTxIrq{status, driverStatus_.nrfState, driverStatus_.nrfTxQueue.size()});
    } else {
        if (driverStatus_.nrfReceiveAfterTransmit) {
            driverStatus_.nrfState = NRFState::Receiver;
            nrf_.enableReceiver();
        } else {
            driverStatus_.nrfState = NRFState::Standby;
            nrf_.standby();
        }
        events_.send(NRFTxIrq{status, driverStatus_.nrfState, driverStatus_.nrfTxQueue.size()});
    }
}

void RCKid::initializeISRs() {

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
}

void RCKid::initializeLibevdevGamepad() {
    gamepadDev_ = libevdev_new();
    libevdev_set_name(gamepadDev_, LIBEVDEV_GAMEPAD_NAME);
    libevdev_set_id_bustype(gamepadDev_, BUS_USB);
    libevdev_set_id_vendor(gamepadDev_, 0x0ada);
    libevdev_set_id_product(gamepadDev_, 0xbabe);
    // enable keys for the buttons and the axes (dpad, thumb, accel)
    libevdev_enable_event_type(gamepadDev_, EV_KEY);
    libevdev_enable_event_type(gamepadDev_, EV_ABS);

    libevdev_enable_event_code(gamepadDev_, EV_KEY, RETROARCH_HOTKEY_ENABLE, nullptr);

    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnA_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnB_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnX_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnY_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnL_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnR_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnSelect_.evdevId, nullptr);
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnStart_.evdevId, nullptr);
    // dpad
    input_absinfo thumb{
        .value = 0,
        .minimum = -1,
        .maximum = 1,
        .fuzz = 0,
        .flat = 0,
        .resolution = 1
    };

    libevdev_enable_event_code(gamepadDev_, EV_ABS, btnDpadUp_.evdevId, &thumb);
    libevdev_enable_event_code(gamepadDev_, EV_ABS, btnDpadLeft_.evdevId, &thumb);
    // thumbstick button
    libevdev_enable_event_code(gamepadDev_, EV_KEY, btnJoy_.evdevId, nullptr);

    // enable the thumbstick and accelerometer
    input_absinfo info {
        .value = 128,
        .minimum = 0,
        .maximum = 255,
        .fuzz = 0,
        .flat = 0,
        .resolution = 1,
    };
    libevdev_enable_event_code(gamepadDev_, EV_ABS, thumbX_.evdevId, & info);
    libevdev_enable_event_code(gamepadDev_, EV_ABS, thumbY_.evdevId, & info);
    libevdev_enable_event_code(gamepadDev_, EV_ABS, accelX_.evdevId, & info);
    libevdev_enable_event_code(gamepadDev_, EV_ABS, accelY_.evdevId, & info);


    int err = libevdev_uinput_create_from_device(gamepadDev_,
                                            LIBEVDEV_UINPUT_OPEN_MANAGED,
                                            &gamepad_);
    if (err != 0) 
        TraceLog(LOG_ERROR, STR("Unable to setup gamepad (result " << err << ", errno: " << errno << ")"));
}

void RCKid::initializeAvr() {
    // check if the AVR is present at all
    if (!i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, nullptr, 0))
        TraceLog(LOG_ERROR, STR("AVR not found:" << errno));
    // check if the AVR is in bootloader mode
    try {
        Programmer pgm{AVR_I2C_ADDRESS, RPI_PIN_AVR_IRQ};
        pgm.setLogLevel(Programmer::LOG_TRACE);
        while (true) {
            ChipInfo ci{pgm.getChipInfo()};
            if (ci.state.status != bootloader::BOOTLOADER_MODE)
                break;
            TraceLog(LOG_WARNING, "AVR in bootloader mode");
            pgm.resetToApp();
            cpu::delayMs(500);
        }
    } catch (std::exception const & e) {
        TraceLog(LOG_ERROR, STR("Cannot talk to AVR: " << e.what()));
    }
    // we are now in non-bootloader mode, check all is good
    comms::Status status = avrQueryStatus();
    TraceLog(LOG_INFO, STR("Power-on AVR status: " << status).c_str());
    // if we are to terminate immediately because the current status is power down, shutdown and terminate itself immediately
    if (status.mode() == comms::Mode::PowerDown) {
        TraceLog(LOG_INFO, "Spurious power-up. Powering down immediately");
        system("sudo poweroff");
        exit(EXIT_SUCCESS);
    }
    // attach the interrupt
    gpio::inputPullup(PIN_AVR_IRQ);
    gpio::attachInterrupt(PIN_AVR_IRQ, gpio::Edge::Falling, & isrAvrIrq);

}

void RCKid::initializeAccel() UI_THREAD {
    if (accel_.deviceIdentification() == 104) {
        accel_.reset();
    } else {
        TraceLog(LOG_ERROR, "Accel not found");
    }
}

void RCKid::initializeNrf() UI_THREAD {
    if (nrf_.initialize("RCKID", "RCKID")) {
        nrf_.standby();
        nrfState_ = NRFState::Standby;
    } else {
        TraceLog(LOG_ERROR, "Radio not found");
        nrfState_ = NRFState::Error;
    }
    nrfTxQueueSize_ = 0;
    // attach the interrupt handler
    gpio::input(PIN_NRF_IRQ);
    gpio::attachInterrupt(PIN_NRF_IRQ, gpio::Edge::Falling, & isrNrfIrq);
}

void RCKid::buttonAction(ButtonState & btn) ISR_THREAD DRIVER_THREAD {
    btn.reported = btn.current;
    btn.autorepeat = BTN_AUTOREPEAT_DURATION;
    if (activeDevice_ != nullptr) {
        if (btn.evdevId != KEY_RESERVED) {
            if (btn.axisValue == 0)
                libevdev_uinput_write_event(activeDevice_, EV_KEY, btn.evdevId, btn.reported ? 1 : 0);
            else
                libevdev_uinput_write_event(activeDevice_, EV_ABS, btn.evdevId, btn.reported ? btn.axisValue : 0);
            libevdev_uinput_write_event(activeDevice_, EV_SYN, SYN_REPORT, 0);
        }
    } else if (btn.reported) {
        sendAvrCommand(msg::Rumbler{96, 10});
    }
    // send the appropriate action to the main thread
    events_.send(ButtonEvent{btn.button, btn.reported});
}





#endif