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
    Error, // Not present
    PowerDown, 
    Standby, 
    Rx,
    Tx,
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

struct SecondTick {};
struct AlarmEvent {};
struct LowBatteryEvent {};
struct StateChangeEvent {};
struct Hearts { uint16_t value; };

/** An event triggered when there is a button change. */
struct ButtonEvent { Button btn; bool state; }; 

struct JoyEvent { uint8_t h; uint8_t v; };

/** The accelerometer readouts. */
struct AccelEvent { uint8_t h; uint8_t v; };

struct HeadphonesEvent { bool connected; }; 

/** Audio recording event. 
 
    Contains the status word which can be used to determine the batch and 32 data bytes of the actual recording. 
 */
struct RecordingEvent { comms::Status status; uint8_t data[32]; };
static_assert(sizeof(RecordingEvent) == 33); 

struct NRFPacketEvent { uint8_t packet[32]; };

struct NRFTxEvent {};

using Event = std::variant<
    comms::Mode, 
    SecondTick,
    AlarmEvent,
    LowBatteryEvent, 
    StateChangeEvent,
    Hearts,
    ButtonEvent, 
    JoyEvent, 
    AccelEvent,
    HeadphonesEvent,
    RecordingEvent,
    NRFPacketEvent,
    NRFTxEvent
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
