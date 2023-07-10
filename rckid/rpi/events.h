#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <variant>

#include "platform/platform.h"
#include "platform/peripherals/nrf24l01.h"
#include "utils/utils.h"

#include "common/comms.h"

/** State of the NRF chip. 
 */
enum class NRFState {
    Error,
    PowerDown, 
    Standby,
    Receiver,
    Transmitting,
};

// helper type for the std::visit deconstruction
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

/** Buttons available on the RCKid. 
 */
enum class Button {
    A, 
    B, 
    X, 
    Y, 
    L, 
    R, 
    Left, 
    Right,
    Up,
    Down, 
    Select, 
    Start, 
    Home, 
    VolumeUp, 
    VolumeDown, 
    Joy, 
}; // Button

/** An event triggered when there is a button change. 
 */
struct ButtonEvent {
    Button btn;
    bool state;
}; // ButtonEvent

struct ThumbEvent {
    uint8_t x;
    uint8_t y;
}; // ThumbEvent

/** The accelerometer readouts.
*/
struct AccelEvent {
    uint8_t x;
    uint8_t y;
    uint8_t z;
    int16_t temp;
};

struct ModeEvent {
    comms::Mode mode;
};

struct ChargingEvent {
    bool usb;
    bool charging;

    bool operator == (ChargingEvent const & other) const { return usb == other.usb && charging == other.charging; }
};

struct VoltageEvent {
    uint16_t vBatt;
    uint16_t vcc;

    bool operator == (VoltageEvent const & other) const { return vBatt == other.vBatt && vcc == other.vcc; }
};

struct TempEvent {
    int16_t temp;

    bool operator == (TempEvent const & other) const { return temp == other.temp; }
};

struct BrightnessEvent {
    uint8_t brightness;
    bool operator == (BrightnessEvent const & other) const { return brightness == other.brightness; }
};

struct HeadphonesEvent {
    bool connected;
}; 

struct UptimeEvent {
    uint32_t uptime;
}; 

/** Audio recording event. 
 
    Contains the status word which can be used to determine the batch and 32 data bytes of the actual recording. 
 */
struct RecordingEvent {
    comms::Status status;
    uint8_t data[32];
};
static_assert(sizeof(RecordingEvent) == 33); 

struct NRFPacketEvent {
    uint8_t packet[32];
};

struct NRFTxIrq { platform::NRF24L01::Status nrfStatus; NRFState newState; size_t txQueueSize; };


using Event = std::variant<
    ButtonEvent,
    ThumbEvent, 
    AccelEvent,
    ModeEvent,
    ChargingEvent,
    VoltageEvent,
    TempEvent, 
    BrightnessEvent,
    HeadphonesEvent,
    UptimeEvent,
    RecordingEvent,
    NRFPacketEvent,
    NRFTxIrq
>;

/** Event queue.
 */
template<typename T> 
class EventQueue {
public:

    /** Sends new event to the queue. 
     */
    void send(T && event) {
        std::lock_guard<std::mutex> g{m_};
        q_.push(std::move(event));
        cv_.notify_all();
    }

    std::optional<T> receive() {
        std::lock_guard<std::mutex> g{m_};
        if (!q_.empty()) {
            auto x = std::move(q_.front());
            q_.pop();
            return x;
        } else {
            return {};
        }
    }

    /** Returns the number of events currently in the queue and if the queue is not empty, pops the front and returns it. 
     
        I.e. if the result != 0, the event is valid. If the result is 1 no need to call again immediately. 
    */
    size_t tryReceive(Event & event) {
        std::lock_guard<std::mutex> g{m_};
        size_t result = q_.size();
        if (result > 0) {
            event = std::move(q_.front());
            q_.pop();
        }
        return result;
    }

    /** Returns next event, if the queue is empty waits for new event to be sent.
     */
    T waitReceive() {
        std::unique_lock<std::mutex> g{m_};
        cv_.wait(g, [this](){ return ! q_.empty(); });
        auto x = std::move(q_.front());
        q_.pop();
        return x;
    }

private:
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable cv_;
}; // EventQueue