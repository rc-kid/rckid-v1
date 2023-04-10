#pragma once

#include <cmath>
#include <functional>

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

    bool update(float deltaMs) {
        if (!running_)
            return false;
        value_ += deltaMs;
        if (value_ > duration_) {
            value_ -= duration_;
            if (! continuous_)
                running_ = false;
            return true;
        } else {
            return false;
        }
    }

    void start() {
        value_ = 0;
        running_ = true;
        continuous_ = false;
    }

    void startContinuous() {
        value_ = 0;
        running_ = true;
        continuous_ = true;
    }

    void stop() {
        running_ = false;
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
        switch (pol) {
            case Interpolation::Linear:
                return static_cast<T>((end - start) * (value / max) + start); 
            case Interpolation::Sin:
                return static_cast<T>((end - start) * sin(PI / 2 * (value / max + start)));
            default:
                UNREACHABLE;
        }
    }

    bool running_ = false;
    bool continuous_ = false;
    float duration_ = 0;
    float value_ = 0;

}; // Animation
