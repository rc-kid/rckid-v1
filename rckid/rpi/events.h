#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <condition_variable>

/** RCKid controls. */
enum class Control {
    A, 
    B, 
    X, 
    Y, 
    L, 
    R, 
    Select, 
    Start, 
    LVol, 
    RVol, 
    Left, 
    Right, 
    Up, 
    Down, 
    Thumb,
    ThumbH, 
    ThumbV,
    Photo,
    AccelX,
    AccelY,
}; // Button

struct Event {
    

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
        q_.push_back(std::move(event));
        cv_.notify_all();
    }

    std::optional<T> receive() {
        std::lock_guard<std::mutex> g{m_};
        if (!q_.empty()) {
            auto x = std::move(q_.front());
            q_.pop_front();
            return x;
        } else {
            return {};
        }
    }

    /** Returns next event, if the queue is empty waits for new event to be sent.
     */
    T waitReceive() {
        std::unique_lock<std::mutex> g{m_};
        cv_.wait(g, [this](){ return ! q_.empty(); });
        auto x = std::move(q_.front());
        q_.pop_front();
        return x;
    }

private:
    std::deque<T> q_;
    std::mutex m_;
    std::condition_variable cv_;
}; // EventQueue