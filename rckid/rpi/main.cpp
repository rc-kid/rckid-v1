#include "gui/raylib_cpp.h"

#include "platform/platform.h"

#include "rckid.h"
#include "gui/gui.h"
#include "gui/menu.h"
//#include "gui/carousel.h"
#include "gui/pixel_editor.h"
#include "gui/debug_view.h"
#include "gui/keyboard.h"


/** Main RCKid app. 
 
    TODO start main loop and do some nice and efficient event loop for the main thread

 */
int main(int argc, char * argv[]) {
    // initialize raylib
    InitWindow(320, 240, "RCKid");
    SetTargetFPS(60);

    // initialize the joystick and peripherals, filling the error messages
    RCKid * rckid = RCKid::initialize();

    // test mode and other important commands
    if (argc == 2 && strcmp(argv[1], "--test") == 0)
        return EXIT_SUCCESS;

    GUI gui;
    Menu menu{
        new Menu::Item{"Games", "assets/images/001-game-controller.png"},
        new Menu::Item{"Remote", "assets/images/002-rc-car.png"},
        new Menu::Item{"Video", "assets/images/005-film-slate.png"},
        new Menu::Item{"Music", "assets/images/003-music.png"},
        new Menu::Item{"Walkie-Talkie", "assets/images/007-baby-monitor.png"},
        new SubmenuItem{"Apps", "assets/images/023-presents.png", {
            new Menu::Item{"Torchlight", "assets/images/004-flashlight.png"},
            new WidgetItem{"Paint", "assets/images/021-poo.png", new PixelEditor{}},
            new Menu::Item{"Baby Monitor", "assets/images/006-baby-crib.png"},
        }},
    };
    //Keyboard kb{};
    //gui.setWidget(&kb);
    gui.setMenu(& menu, 0);
    gui.loop(rckid);
}