#pragma once

#include <cmath>

#include "platform/platform.h"

enum class Interpolation {
    Linear, 
    Sin,
}; // Interpolation

/** A collection of helper functions for widgets. 
*/
class WidgetHelper {
public:


    static int interpolate(int start, int end, double pct, Interpolation pol = Interpolation::Sin) {
        switch (pol) {
            case Interpolation::Linear:
                return static_cast<int>((end - start) * pct + start); 
            case Interpolation::Sin:
                return static_cast<int>((end - start) * sin(PI / 2 * pct) + start);
            default:
                UNREACHABLE;
        }
    }


    template<typename T>
    static int interpolate(int start, int end, T value, T max, Interpolation pol = Interpolation::Sin) {
        switch (pol) {
            case Interpolation::Linear:
                return static_cast<int>((end - start) * ((float)value / max) + start); 
            case Interpolation::Sin:
                return static_cast<int>((end - start) * sin(PI / 2 * ((float)value / max) + start));
            default:
                UNREACHABLE;
        }
    }

    /** Interpolates between start .. end .. start*/
    template<typename T>
    static int interpolateContinous(int start, int end, T value, T max, Interpolation pol = Interpolation::Sin) {
        float fvalue = (float)value;
        float fmax = (float)max;
        float fhalf = fmax / 2;
        if (fvalue >= fhalf)
            return interpolate(end, start, fvalue - fhalf, fhalf, pol);
        else
            return interpolate(start, end, fvalue, fhalf, pol);
    }  

/*
    template<typename T> 
    static int interploateContinuous(int start, int end, T value, T max, Interpolation pol = Interpolation::Sin {

    }
*/

}; // WidgetHelper

