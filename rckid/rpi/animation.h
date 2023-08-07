#pragma once

#include <cmath>
#include <functional>
#include <cmath>

#include "platform/platform.h"


enum class Interpolation {
    Linear, 
    Sin,
}; // Interpolation


class Animation {
public:

    Animation(float duration):
        duration_{duration} {
    }

    /** Duration of the animation in milliseconds. 
     */
    float duration() const { return duration_; }

    void setDuration(float ms) { duration_ = ms; }

    /** Updates the aniatimation. Returns true if the animation's tick has elapsed since last call, false otherwise. If the animation is in a single shot mode, stops the amination if tick has passed. If the animation is not running, returns always false. 
     */
    bool update();

    bool running() const { return running_; }
    
    void start() {
        value_ = 0;
        running_ = true;
        continuous_ = false;
        done_ = false;
    }

    void start(float duration) {
        duration_ = duration;
        start();
    }

    void startContinuous() {
        value_ = 0;
        running_ = true;
        continuous_ = true;
    }

    void stop() {
        running_ = false;
    }

    /** Returns true if the last update of the animation marked its end. 
     
        Note that for continuous animation, the animation will restart after done. 
     */
    bool done() const {
        return done_;
    }

    float frame() const { return value_; }

    float fraction() const { return value_ / duration_; }

    template<typename T> T interpolate(T start, T end, Interpolation pol = Interpolation::Sin) {
        return interpolate(start, end, value_, duration_, pol);
    }

    /** Interpolates between start .. end .. start*/
    template<typename T> T interpolateContinuous(T start, T end, Interpolation pol = Interpolation::Sin) {
        float half = duration_ / 2;
        if (value_ >= half)
            return interpolate(end, start, value_ - half, half, pol);
        else
            return interpolate(start, end, value_, half, pol);
    }

private:

    template<typename T> T interpolate(T start, T end, float value, float max, Interpolation pol) {
        float f = value / max;
        if (end < start) {
            std::swap(start, end);
            f = 1 - f;
        }
        switch (pol) {
            case Interpolation::Linear:
                return static_cast<T>((end - start) * f + start); 
            case Interpolation::Sin:
                return static_cast<T>((end - start) * sin(M_PI / 2 * (f + start)));
            default:
                UNREACHABLE;
        }
    }

    bool running_ = false;
    bool continuous_ = false;
    bool done_ = false;
    float duration_ = 0;
    float value_ = 0;

}; // Animation

/** A simple timer.  
*/
class Timer {
public:
    bool update();

    bool running() const { return running_; }

    void start(float duration) {
        duration_ = duration;
        value_ = 0;
        running_ = true;
        singleShot_ = true;
    }

    void start() { start(duration_); }

    /** Starts the timer with a random duration from the min-max interval in milliseconds. 
     */
    void startRandom(float min, float max) {
        float  d = min + (max - min) * (rand() % 10000) / 10000;
        start(d);
    }

    void startContinuous(float duration) {
        duration_ = duration;
        value_ = 0;
        running_ = true;
        singleShot_ = false;
    } 

    void startContinuous() { startContinuous(duration_); }

    void stop() {
        running_ = false;
    }

    bool done() const { return value_ >= duration_; }

private:
    bool running_ = false;
    bool singleShot_ = false;
    float duration_ = 0;
    float value_ = 0;
}; 