#include <thread>

/** A simple spinlock. 
 
    These are useful in ISRs where a normal blocking mutex cannot be used. 
 */
class SpinLock {
public:
    void lock() {
        while (f_.test_and_set(std::memory_order_acquire)) {
            // spin
        }
    }

    void unlock() {
        f_.clear(std::memory_order_release);
    }

private:

    std::atomic_flag f_ = ATOMIC_FLAG_INIT;
};