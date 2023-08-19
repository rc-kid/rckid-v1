#include <iostream>

#include "../avr-i2c-bootloader/src/programmer.h"

#include "raylib_wrapper.h"

#include "rckid.h"



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
        {
            std::lock_guard<std::mutex> g{mState_};
            i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& state_, sizeof(state_));
            // don't process the state - it's the first state and hence the initial value. Setting the mode to on and then processing the status calls ensures the transition to on state will be completed if necessary
            state_.status.setMode(comms::Mode::On);
            processAvrStatus(state_.status, true);
        }
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
            this->uiEvents_.send(SecondTick{});
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
    if (e) {
        std::visit(overloaded{
            [this](SecondTick const &) {
                if (heartsCounterEnabled_ > 0) {
                    if (pState_.hearts > 0)
                        --pState_.hearts;
                    uiEvents_.send(Hearts{pState_.hearts});
                }
            },
            // defaukt case does nothing
            [](auto const &) {}
        }, e.value());

    }
    return e;
}

void RCKid::powerOff() {
    TraceLog(LOG_INFO, "Power down initiated");
    writePersistentState();
    driverEvents_.send(msg::PowerDown{});
}

void RCKid::startAudioRecording() {
    TraceLog(LOG_DEBUG, "Recording start");
    driverEvents_.send(msg::StartAudioRecording{});
    // notify the main thread that there has been a state change (amongst other things refreshes the header)
    uiEvents_.send(StateChangeEvent{});
#if (defined ARCH_MOCK)
    mockRecBatch_ = 0;
    mockRecording_ = true;
#endif
}

void RCKid::stopAudioRecording() {
    TraceLog(LOG_DEBUG, "Recording stopped");
    driverEvents_.send(msg::StopAudioRecording{}); 
    // notify the main thread that there has been a state change (amongst other things refreshes the header)
    uiEvents_.send(StateChangeEvent{});
#if (defined ARCH_MOCK)
    mockRecording_ = false;
#endif
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
            buttonTick(btnJoyUp_);
            buttonTick(btnJoyDown_);
            buttonTick(btnJoyLeft_);
            buttonTick(btnJoyRight_);
            buttonTick(btnAccelUp_);
            buttonTick(btnAccelDown_);
            buttonTick(btnAccelLeft_);
            buttonTick(btnAccelRight_);
            // only query the accell and photores status when we are not recording to keep the I2C fully for the audio recorder
            if (!state_.status.recording()) {
                queryAccelStatus();
                // TODO query photores
            }
        },
        [this](SecondTick) {
            // only query extended state if we are not recording audio at the same time
            if (!state_.status.recording()) {
                comms::ExtendedState state{queryAvrExtendedState()};
                std::lock_guard<std::mutex> g{mState_};
                processAvrStatus(state.status, true);
                processAvrControls(state.controls, true);
                processAvrExtendedState(state, true);
            }
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
            {
                std::lock_guard<std::mutex> g{mRadio_};
                nrfState_ = e;
            }
            // notify the main thread that there has been a state change (amongst other things refreshes the header)
            uiEvents_.send(StateChangeEvent{});
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
        // immediate transmit
        [this](NRFPacket e) {
            nrfTx_ = true;
            nrf_.transmit(e.packet, 32);
            nrf_.enablePolledTransmitter();
            NRF24L01::Status status = nrf_.clearTxIrqs();
            while (!status.txDataSentIrq() && !status.txDataFailIrq())
                status = nrf_.clearTxIrqs();
            nrf_.enableReceiver();
            nrfTx_ = false;            
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
    // get the persistent state
    readPersistentState();
    // update brightness and volume
    setBrightness(pState_.brightness);
    setVolume(pState_.volume);
    // attach the interrupt
    gpio::inputPullup(PIN_AVR_IRQ);
    gpio::attachInterrupt(PIN_AVR_IRQ, gpio::Edge::Falling, & isrAvrIrq);
}

void RCKid::readPersistentState() {
    i2c::transmit(AVR_I2C_ADDRESS, & msg::GetPersistentState::ID, 1, nullptr, 0);
    uint8_t buf[sizeof(comms::PersistentState) + 1];
    i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, buf, sizeof(buf));
    comms::PersistentState * pState = reinterpret_cast<comms::PersistentState*>(buf + 1);
    TraceLog(LOG_INFO, "Persistent state from AVR:");
    TraceLog(LOG_INFO, STR("alarm:"));
    TraceLog(LOG_INFO, STR("brightness: " << (int) pState->brightness));
    TraceLog(LOG_INFO, STR("volume:     " << (int) pState->volume));
    TraceLog(LOG_INFO, STR("hearts:     " << (int) pState->hearts));
    TraceLog(LOG_INFO, STR("joyHMin:    " << (int) pState->joyHMin));
    TraceLog(LOG_INFO, STR("joyHMax:    " << (int) pState->joyHMax));
    TraceLog(LOG_INFO, STR("joyVMin:    " << (int) pState->joyVMin));
    TraceLog(LOG_INFO, STR("joyVMax:    " << (int) pState->joyVMax));
    pState_ = *pState;
}

void RCKid::writePersistentState() {
    driverEvents_.send(msg::SetPersistentState{pState_});
}

void RCKid::processAvrStatus(comms::Status status, bool alreadyLocked) {
    utils::cond_lock_guard g{mState_, alreadyLocked};
    if (state_.status.mode() != status.mode()) {
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
    if (state_.status.powerStatus() != status.powerStatus()) {
        state_.status.setPowerStatus(status.powerStatus());
        uiEvents_.send(status.powerStatus());
    }
}

void RCKid::processAvrControls(comms::Controls controls, bool alreadyLocked) {
    utils::cond_lock_guard g{mState_, alreadyLocked};
    if (state_.controls.select() != controls.select()) {
        state_.controls.setSelect(controls.select());
        if (btnSelect_.update(state_.controls.select()))
            buttonAction(btnSelect_, true);
    }
    if (state_.controls.start() != controls.start()) {
        state_.controls.setStart(controls.start());
        if (btnStart_.update(state_.controls.start()))
            buttonAction(btnStart_, true);
    }
    if (state_.controls.triggerLeft() != controls.triggerLeft()) {
        state_.controls.setTriggerLeft(controls.triggerLeft());
        if (btnL_.update(state_.controls.triggerLeft()))
            buttonAction(btnL_, true);
    }
    if (state_.controls.triggerRight() != controls.triggerRight()) {
        state_.controls.setTriggerRight(controls.triggerRight());
        if (btnR_.update(state_.controls.triggerRight()))
            buttonAction(btnR_, true);
    }
    if (state_.controls.home() != controls.home()) {
        state_.controls.setButtonHome(controls.home());
        if (btnHome_.update(state_.controls.home()))
            buttonAction(btnHome_, true);
    }

    // joystick reading
    bool report = false;
    if (joyX_.update(controls.joyH())) {
        if (joyAsButtons_) {
            AnalogButtonState bState{axisAsButton(controls.joyH(), ANALOG_BUTTON_THRESHOLD)};
            if (btnJoyLeft_.update(bState == AnalogButtonState::Low))
                buttonAction(btnJoyLeft_, true);
            if (btnJoyRight_.update(bState == AnalogButtonState::High))
                buttonAction(btnJoyRight_, true);
        } else {
            report = true;
            axisAction(joyX_, true);
        }
        state_.controls.setJoyH(controls.joyH());
    }
    if (joyY_.update(controls.joyV())) {
        if (joyAsButtons_) {
            AnalogButtonState bState{axisAsButton(controls.joyV(), ANALOG_BUTTON_THRESHOLD)};
            if (btnJoyUp_.update(bState == AnalogButtonState::Low))
                buttonAction(btnJoyUp_, true);
            if (btnJoyDown_.update(bState == AnalogButtonState::High))
                buttonAction(btnJoyDown_, true);
        } else {
            report = true;
            axisAction(joyY_, true);
        }
        state_.controls.setJoyV(controls.joyV());
    }
    if (report)
        uiEvents_.send(JoyEvent{joyX_.reportedValue, joyY_.reportedValue});
}

void RCKid::processAvrExtendedState(comms::ExtendedState & state, bool alreadyLocked) {
    utils::cond_lock_guard g{mState_, alreadyLocked};
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
    // simply copy the time and uptime, we mostly use them for debugging purposes
    state_.uptime = state.uptime;
    state_.time = state.time;
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
        if (accelAsButtons_) {
            AnalogButtonState bState{axisAsButton(x, ANALOG_BUTTON_THRESHOLD)};
            if (btnAccelUp_.update(bState == AnalogButtonState::Low))
                buttonAction(btnAccelUp_, true);
            if (btnAccelDown_.update(bState == AnalogButtonState::High))
                buttonAction(btnAccelDown_, true);
        } else {
            report = true;
            axisAction(accelX_);
        }
    }
    if (accelY_.update(y)) {
        if (accelAsButtons_) {
            AnalogButtonState bState{axisAsButton(y, ANALOG_BUTTON_THRESHOLD)};
            if (btnAccelRight_.update(bState == AnalogButtonState::Low))
                buttonAction(btnAccelRight_, true);
            if (btnAccelLeft_.update(bState == AnalogButtonState::High))
                buttonAction(btnAccelLeft_, true);
        } else {
            report = true;
            axisAction(accelY_);
        }
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
    gpio::attachInterrupt(PIN_BTN_DPAD_UP, gpio::Edge::Both, & isrButtonDpadUp);
    gpio::attachInterrupt(PIN_BTN_DPAD_DOWN, gpio::Edge::Both, & isrButtonDpadDown);
    gpio::attachInterrupt(PIN_BTN_DPAD_LEFT, gpio::Edge::Both, & isrButtonDpadLeft);
    gpio::attachInterrupt(PIN_BTN_DPAD_RIGHT, gpio::Edge::Both, & isrButtonDpadRight);
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

