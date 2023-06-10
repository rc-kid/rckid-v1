#include <cstdlib>
#include <iostream>

#include "utils/time.h"
#include "raylib.h"



static constexpr int GLYPHS[] = {
    32, 33, 34, 35, 36,37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, // space & various punctuations
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, // 0..9
    58, 59, 60, 61, 62, 63, 64, // more punctuations
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, // A-Z
    91, 92, 93, 94, 95, 96, // more punctuations
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, // a-z
    123, 124, 125, 126, // more punctiations
    0xf004, 0xf08a, //  
    0xf05a9, 0xf05aa, 0xf16c1, // 󰖩 󰖪 󱛁
    0xf244, 0xf243, 0xf242, 0xf241, 0xf240, 0xf0e7, //      
    0xf02cb, // 󰋋
    0xf1119, // 󱄙
    0xf0e08, 0xf057f, 0xf0580, 0xf057e, // 󰸈 󰕿 󰖀 󰕾
};


#define NUM_FRAMES 100

int main(int argc, char* argv[]) {
    Timepoint t = now();
    srand(43);
    InitWindow(320, 240, "rbench");
    if (! IsWindowReady())
        TraceLog(LOG_ERROR, "Unable to initialize window");
    Font f = LoadFontEx("assets/fonts/", 32, const_cast<int*>(GLYPHS), sizeof(GLYPHS) / sizeof(int));
    Texture2D tt = LoadTexture("assets/images/012-airplane-mode.png");
    RenderTexture2D rt = LoadRenderTexture(320, 240);
    std::cout << "Initialized in " << asMillis(now() - t) << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(tt, 30, 30, WHITE);
        EndDrawing();
        SwapScreenBuffer();
    }
    std::cout << "Simple frame: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (size_t x = 0; x < 1000; ++x) 
            DrawTexture(tt, rand() % 320, rand() % 240, ColorAlpha(WHITE, 0.5));
        EndDrawing();
        SwapScreenBuffer();
    }
    std::cout << "1k textures: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (size_t x = 0; x < 1000; ++x) 
            DrawTextEx(f, "foobar", Vector2{float(rand() % 320), float(rand() % 240)}, 32, 1, ColorAlpha(WHITE, 0.5));
        EndDrawing();
        SwapScreenBuffer();
    }
    std::cout << "1k text: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (size_t x = 0; x < 1000; ++x) 
            DrawCircleLines(float(rand() % 320), float(rand() % 240), 32, ColorAlpha(WHITE, 0.5));
        EndDrawing();
        SwapScreenBuffer();
    }
    std::cout << "1k circles: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (size_t x = 0; x < 1000; ++x) 
            DrawRectangle(rand() % 320, rand() % 240, rand() % 100, rand() % 100, ColorAlpha(WHITE, 0.5));
        EndDrawing();
        SwapScreenBuffer();
    }
    std::cout << "1k rectangles: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginTextureMode(rt);
        ClearBackground(BLACK);
        DrawTexture(tt, 30, 30, WHITE);
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(rt.texture, 0, 0, WHITE);
        EndDrawing();
        SwapScreenBuffer();
    }
    std::cout << "RT simple frame: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginTextureMode(rt);
        ClearBackground(BLACK);
        for (size_t x = 0; x < 1000; ++x) 
            DrawTexture(tt, rand() % 320, rand() % 240, ColorAlpha(WHITE, 0.5));
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(rt.texture, 0, 0, WHITE);
        EndDrawing();
        SwapScreenBuffer();

    }
    std::cout << "RT 1k textures: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginTextureMode(rt);
        ClearBackground(BLACK);
        for (size_t x = 0; x < 1000; ++x) 
            DrawTextEx(f, "foobar", Vector2{float(rand() % 320), float(rand() % 240)}, 32, 1, ColorAlpha(WHITE, 0.5));
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(rt.texture, 0, 0, WHITE);
        EndDrawing();
        SwapScreenBuffer();

    }
    std::cout << "RT 1k text: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginTextureMode(rt);
        ClearBackground(BLACK);
        for (size_t x = 0; x < 1000; ++x) 
            DrawCircleLines(float(rand() % 320), float(rand() % 240), 32, ColorAlpha(WHITE, 0.5));
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(rt.texture, 0, 0, WHITE);
        EndDrawing();
        SwapScreenBuffer();

    }
    std::cout << "RT 1k circles: " << asMillis(now() - t) / NUM_FRAMES << std::endl;

    t = now();
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        BeginTextureMode(rt);
        ClearBackground(BLACK);
        for (size_t x = 0; x < 1000; ++x) 
            DrawRectangle(rand() % 320, rand() % 240, rand() % 100, rand() % 100, ColorAlpha(WHITE, 0.5));
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(rt.texture, 0, 0, WHITE);
        EndDrawing();
        SwapScreenBuffer();

    }
    std::cout << "RT 1k rectangles: " << asMillis(now() - t) / NUM_FRAMES << std::endl;






    return EXIT_SUCCESS;
}