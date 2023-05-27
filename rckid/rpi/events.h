#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <variant>

#include "platform/platform.h"
#include "utils/utils.h"

#include "common/comms.h"

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

using Event = std::variant<
    ButtonEvent,
    ThumbEvent, 
    AccelEvent,
    ModeEvent,
    ChargingEvent,
    VoltageEvent,
    TempEvent, 
    BrightnessEvent,
    HeadphonesEvent
>;


#ifdef FOOBAR

/** Window event.

    The window event is effectively a tagged union over the various event types supported by the window.  
 */
struct Event {
public:
    enum class Kind {
        None, 
        Button, 
        Thumb,
        Accel,
        State, 
    }; // Event::Kind

    Kind kind;

    Event():kind{Kind::None} {}

    static Event button(Button btn, bool state) {
        Event result{Kind::Button};
        result.button_.btn = btn;
        result.button_.state = state;
        return result;
    }

    static Event accel(uint8_t x, uint8_t y, uint8_t z, uint16_t temp) {
        Event result{Kind::Accel};
        result.accel_.x = x;
        result.accel_.y = y;
        result.accel_.z = z;
        result.accel_.temp = temp;
        return result;
    }

    ButtonEvent & button() {
        ASSERT(kind == Kind::Button);
        return button_;
    }

    ThumbEvent & thumb() {
        ASSERT(kind == Kind::Thumb);
        return thumb_;
    }

    AccelEvent & accel() {
        ASSERT(kind == Kind::Accel);
        return accel_;
    }

    StateEvent & state() {
        ASSERT(kind == Kind::State);
        return state_;
    }

private:

    Event(Kind kind): kind{kind} {}

    union {
        ButtonEvent button_;
        ThumbEvent thumb_;
        AccelEvent accel_;
        StateEvent state_;
    }; 

}; // Event


#endif // FOOBAR

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