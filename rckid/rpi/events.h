#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "platform/platform.h"

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

/** Window event.

    The window event is effectively a tagged union over the various event types supported by the window.  
 */
struct Event {
public:
    enum class Kind {
        None, 
        Button, 
    }; // Event::Kind

    Kind kind;

    Event():kind{Kind::None} {}

    Event(Button button, bool state):kind{Kind::Button}, button_{button, state} {}

    ButtonEvent & button() {
        ASSERT(kind == Kind::Button);
        return button_;
    }

private:

    union {
        ButtonEvent button_;
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