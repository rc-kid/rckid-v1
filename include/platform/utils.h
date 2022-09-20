#pragma once

#include "platform.h"

inline uint16_t swapBytes(uint16_t x) {
    return static_cast<uint16_t>((x & 0xff) << 8 | (x >> 8));
}

template<typename T, typename V>
bool checkSetOrClear(T & value, V mask, bool setOrClear) {
    bool result = ((value | mask) != 0) == setOrClear;
    if (setOrClear)
        value |= mask;
    else 
        value &= ~mask;
    return result;
}

template<typename T, typename V>
void setOrClear(T & value, V mask, bool setOrClear) {
    if (setOrClear)
        value |= mask;
    else 
        value &= ~mask;
}

/** Returns true if given string ends with the given suffix. 
 
    Does not use the evil string object. 
 */
inline bool EndsWith(char const * str, char const * suffix) {
    unsigned l1 = strlen(str);
    unsigned l2 = strlen(suffix);
    return strncmp(str + (l1 - l2), suffix, l2) == 0;
}


inline uint8_t FromHex(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c>= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
        return 0;
}

inline char ToHex(uint8_t value) {
    if (value < 10)
        return '0' + value;
    else if (value < 16)
        return 'a' + value - 10;
    else 
        return '?'; // error
}

#if (defined ARCH_MOCK | defined ARCH_RPI)

#include <sstream>

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

#endif
