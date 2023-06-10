#pragma once

#include <chrono>

using Timepoint = std::chrono::high_resolution_clock::time_point;
using Duration = std::chrono::high_resolution_clock::duration;

inline Timepoint now() {
    return std::chrono::high_resolution_clock::now();
}

inline int64_t asMillis(Duration d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

inline int64_t asMicros(Duration d) {
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}