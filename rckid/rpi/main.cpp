#include "gui/raylib-cpp.h"

#include "platform/platform.h"

#include "rckid.h"
#include "gui/gui.h"

/** Main RCKid app. 
 
    TODO start main loop and do some nice and efficient event loop for the main thread

 */
int main(int argc, char * argv[]) {
    // initialize raylib
    InitWindow(320, 240, "RCKid");
    SetTargetFPS(60);
    //Texture2D texture = LoadTexture("assets/images/001-game-controller.png"); 
    //Font font = LoadFont("assets/fonts/OpenDyslexic.otf");

    RCKid * rckid = RCKid::initialize();
    GUI gui;
    gui.addFooterItem(RED, "Back");
    gui.addFooterItem(GREEN, "Select");
    while (true) {
        gui.draw();
        if (WindowShouldClose())
            break;

/*        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(texture, 0, 0, WHITE);
        DrawText("this IS a texture!", 0, 100, 10, GRAY);
        EndDrawing();
        */
//        cpu::delay_ms(100);
    }
}