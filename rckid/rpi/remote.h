#pragma once

#include <unordered_map>

#include "widget.h"
#include "window.h"

#include "remote/lego_remote.h"


    struct RemoteDevice {
        std::string name;
        uint16_t id;

        RemoteDevice(): id{0} {}
        RemoteDevice(char const * name, uint16_t id): name{name}, id{id} {}

        bool operator == (RemoteDevice const & other) const { return name == other.name && id == other.id; }
    }; 


    template<>
    struct std::hash<RemoteDevice> {
        size_t operator()(RemoteDevice const & d) const noexcept {
            size_t h1 = std::hash<std::string>{}(d.name);
            size_t h2 = std::hash<uint16_t>{}(d.id);
            return h1 ^ (h2 << 1);
        }
    };

/** Remote controller widget. 
 
    Contains basic connection stuff, but is really intended to be specialized for a more tailored remote experience. Likely. 
 */
class Remote : public Widget {
public:


protected:

    /** Resets the NRF to default remote addresses and channel and starts searching for devices.
     */
    void searchForDevices() {
        using namespace remote::msg;
        rckid().nrfInitialize("RCKID", DefaultAddress, DefaultChannel);
        rckid().nrfEnableReceiver();
        mode_ = Mode::Searching;
        // get device information
        new (msg_) RequestDeviceInfo{"RCKID"};
        rckid().nrfTransmitImmediate(msg_);
        std::cout << "Searching for devices" << std::endl;
        devices_.clear();
        t_.start(100);
        counter_ = 20;
    }

    void pairWith(char const * deviceName, uint16_t deviceId, char const * deviceAddress) {
        using namespace remote::msg;
        std::cout << "Pairing" << std::endl;
        new (msg_) Pair{"RCKid", deviceAddress, 80, deviceId, deviceName};
        for (size_t i = 0; i < 10; ++i)
            rckid().nrfTransmitImmediate(msg_);
        rckid().nrfInitialize("RCKid", deviceAddress, 80);
        rckid().nrfEnableReceiver();
        t_.startContinuous(50);
        mode_ = Mode::Pairing;
        //
        //for (size_t i = 0; i < 32; ++i) 
        //    std::cout << (int)msg_[i] << std::endl;
    }

    void tick() override {
        using namespace remote::msg;
        if (t_.update()) {
            switch (mode_) {
                case Mode::Searching:
                    if (--counter_ == 0) {
                        mode_ = Mode::Select;
                        for (auto const & i : devices_) {
                            std::cout << i.first.name << " (" << i.first.id << "):" << i.second << std::endl;
                            //pairWith(i.first.name.c_str(), i.first.id, "LEGOR");
                        }
                    } else {
                        new (msg_) RequestDeviceInfo{"RCKID"};
                        rckid().nrfTransmitImmediate(msg_);
                        t_.start(100);
                    }
                    break;
                case Mode::Pairing:
                case Mode::Connected:
                    new (msg_) Nop{};
                    rckid().nrfTransmitImmediate(msg_);
                    break;
                default:
                    // nothing to do
                    break;
            }
        }
    }

    void draw() override {
    }

    /** When pushed on the nav stack, start looking for devices to pair with. 
     */
    void onNavigationPush() override {
        searchForDevices();
    }

    void onFocus() override {
    }

    void onBlur() override {
        rckid().nrfPowerDown();
    }

    void btnA(bool state) override {
        /*
        using namespace remote;
        if (state) {
            msg_.ctrl.mode = channel::Motor::Mode::Brake;
            msg_.ctrl.speed = 0;
            rckid().nrfTransmit(reinterpret_cast<uint8_t*>(&msg_), sizeof(msg_));
        } 
        */
    }

    void btnX(bool state) override {
        /*
        using namespace remote;
        if (state) {
            msg_.ctrl.mode = channel::Motor::Mode::CW;
            msg_.ctrl.speed = speed_;
            speed_ += 1;
            rckid().nrfTransmit(reinterpret_cast<uint8_t*>(&msg_), sizeof(msg_));
        } 
        */
    }

    void nrfPacketReceived(NRFPacketEvent & e) override {
        using namespace remote::msg;
        switch (e.packet[0]) {
            case DeviceInfo::ID: {
                DeviceInfo * msg = reinterpret_cast<DeviceInfo*>(e.packet);
                std::cout << "Device info received: " << msg->name << ", id: " << msg->deviceId << " num channels: " << (int) msg->numChannels << std::endl;
                RemoteDevice d{msg->name, msg->deviceId};
                auto i = devices_.find(d);
                if (i == devices_.end())
                    devices_[d] = 1;
                else
                    ++(i->second);
                break;
            }
            default:
                std::cout << "Unknown message received : " << (int) e.packet[0] << std::endl;
                break;
        }
    }

private:

    enum class Mode {
        None, 
        Searching, 
        Select,
        Pairing,
        Connected,
    };

    uint8_t msg_[32];

    Mode mode_ = Mode::None;
    Timer t_;
    size_t counter_;
    std::unordered_map<RemoteDevice, size_t> devices_;


    //LegoRemote::Feedback feedback_;
    uint8_t speed_ = 0;

    /*
    struct {
        uint8_t channel = remote::LegoRemote::CHANNEL_ML;
        remote::channel::Motor::Control ctrl;
        uint8_t eof = 0;
    } __attribute__((packed)) msg_; */

    //static_assert(sizeof(msg_) == 4);


}; // Remote

