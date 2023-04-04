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

}; // WidgetHelper

