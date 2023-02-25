#pragma once

#include "raylib.h"



inline void DrawTextEx(Font font, const char *text, int posX, int posY, float fontSize, float spacing, Color tint) {
    DrawTextEx(font, text, Vector2{(float)posX, (float)posY}, fontSize, spacing, tint);
}

