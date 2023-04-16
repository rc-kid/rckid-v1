#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "platform/platform.h"
#include "utils/utils.h"

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

struct ButtonEvent {
    Button btn;
    bool state;
}; // ButtonEvent

struct BatteryEvent {
    uint16_t vBatt;
    uint16_t vcc;
    bool charging;
};

/** Window event.

    The window event is effectively a tagged union over the various event types supported by the window.  
 */
struct Event {
public:
    enum class Kind {
        None, 
        Button, 
        Battery, 
    }; // Event::Kind

    Kind kind;

    Event():kind{Kind::None} {}

    static Event button(Button btn, bool state) {
        Event result{Kind::Button};
        result.button_.btn = btn;
        result.button_.state = state;
        return result;
    }

    static Event battery(uint16_t vBatt, uint16_t vcc, bool charging) {
        Event result{Kind::Battery};
        result.battery_.vBatt = vBatt;
        result.battery_.vcc = vcc;
        result.battery_.charging = charging;
        return result;
    }

    ButtonEvent & button() {
        ASSERT(kind == Kind::Button);
        return button_;
    }

private:

    Event(Kind kind): kind{kind} {}

    union {
        ButtonEvent button_;
        BatteryEvent battery_;
    }; 

}; // Event


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