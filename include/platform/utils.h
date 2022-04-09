#pragma once
#include "platform.h"

inline uint16_t swapBytes(uint16_t x) {
    return static_cast<uint16_t>((x & 0xff) << 8 | (x >> 8));
}

template<typename T, typename V>
void setOrClear(T & value, V mask, bool setOrClear) {
    if (setOrClear)
        value |= mask;
    else 
        value &= ~mask;
}

