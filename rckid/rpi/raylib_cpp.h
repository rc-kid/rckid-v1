#pragma once

#include <cstdint>
#include <string>
#include "raylib.h"

/** Allows log function to take std::string as well. 
 */
inline void TraceLog(int loglevel, std::string const & str) {
    TraceLog(loglevel, str.c_str());
}

inline void DrawTextEx(Font font, const char *text, int posX, int posY, float fontSize, float spacing, Color tint) {
    DrawTextEx(font, text, Vector2{(float)posX, (float)posY}, fontSize, spacing, tint);
}

inline Vector2 MeasureText(Font font, char const * text, float fontSize, float spacing = 1.0) {
    return MeasureTextEx(font, text, fontSize, spacing);
}

inline constexpr Color RGB(uint8_t r, uint8_t g, uint8_t b) {
    return Color{r, g, b, 255};
}



