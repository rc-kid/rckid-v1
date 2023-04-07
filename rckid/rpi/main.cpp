#include "gui/raylib_cpp.h"

#include "platform/platform.h"

#include "rckid.h"
#include "gui/gui.h"
#include "gui/menu.h"
#include "gui/carousel.h"
#include "gui/pixel_editor.h"

/** Main RCKid app. 
 
    TODO start main loop and do some nice and efficient event loop for the main thread

 */
int main(int argc, char * argv[]) {
    // first initialize the joystick and peripherals, filling the error messages
    RCKid * rckid = RCKid::initialize();

    // test mode and other important commands
    if (argc == 2 && strcmp(argv[1], "--test") == 0)
        return EXIT_SUCCESS;


    // initialize raylib
    InitWindow(320, 240, "RCKid");
    SetTargetFPS(60);
    //Texture2D texture = LoadTexture("assets/images/001-game-controller.png"); 
    //Font font = LoadFont("assets/fonts/OpenDyslexic.otf");

    GUI gui;
    Menu menu{
        Menu::Item{"Games", "assets/images/001-game-controller.png"},
        Menu::Item{"Remote", "assets/images/002-rc-car.png"},
        Menu::Item{"Video", "assets/images/005-film-slate.png"},
        Menu::Item{"Music", "assets/images/003-music.png"},
        Menu::Item{"Walkie-Talkie", "assets/images/007-baby-monitor.png"},
        Menu::Item{"Torchlight", "assets/images/004-flashlight.png"},
    };
    //gui.setMenu(& menu, 0);
    PixelEditor pEditor{&gui};
    gui.setWidget(& pEditor);



    gui.addFooterItem(RED, "Back");
    gui.addFooterItem(GREEN, "Select");
    while (true) {
        gui.processInputEvents();
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