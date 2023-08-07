#pragma once

#include <raylib.h>

/** Allows log function to take std::string as well. 
 */
inline void TraceLog(int loglevel, std::string const & str) {
    TraceLog(loglevel, str.c_str());
}

/** Creates an opaque color from its constituent parts. 
 */
inline constexpr Color RGB(uint8_t r, uint8_t g, uint8_t b) {
    return ::Color{r, g, b, 255};
}

/** Creates an RGB color with given opacity (0 == fully transparent, 255 = fully opaque). 
 */
inline constexpr Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ::Color{r, g, b, a};
}

inline constexpr Vector2 V2(float x, float y) { return Vector2{x, y}; }
inline constexpr Vector2 V2(int x, int y) { return Vector2{static_cast<float>(x), static_cast<float>(y)}; }

inline constexpr Rectangle RECT(int x, int y, int width, int height) { 
    return Rectangle{
        static_cast<float>(x), 
        static_cast<float>(y), 
        static_cast<float>(width), 
        static_cast<float>(height), 
    };
}


/*
#include <cstdint>
#include <string>
#include <filesystem>
#include "raylib.h"

*/
/** Allows log function to take std::string as well. 
 */
/*
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

inline constexpr Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return Color{r, g, b, a};
}

inline constexpr Vector2 V2(float x, float y) { return Vector2{x, y}; }
inline constexpr Vector2 V2(int x, int y) { return Vector2{static_cast<float>(x), static_cast<float>(y)}; }



inline constexpr Rectangle RECT(int x, int y, int width, int height) { 
    return Rectangle{
        static_cast<float>(x), 
        static_cast<float>(y), 
        static_cast<float>(width), 
        static_cast<float>(height), 
    };
}

*/

/*
inline Music LoadMusicStream(std::filesystem::path const & path) {
    return LoadMusicStream(path.c_str());
}
*/


