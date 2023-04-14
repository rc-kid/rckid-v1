#include "raylib_cpp.h"

#include "platform/platform.h"

#include "rckid.h"
#include "gui.h"
#include "menu.h"
//#include "gui/carousel.h"
#include "pixel_editor.h"
#include "debug_view.h"
#include "keyboard.h"
#include "game_player.h"


/** Main RCKid app. 
 
    TODO start main loop and do some nice and efficient event loop for the main thread

 */
int main(int argc, char * argv[]) {

    GUI gui;
    gui.startRendering();
    Menu menu{&gui, {
        //new Menu::Item{"Games", "assets/images/001-game-controller.png"},
        new WidgetItem{"Games", "assets/images/001-game-controller.png", new GamePlayer{&gui}},
        new Menu::Item{"Remote", "assets/images/002-rc-car.png"},
        new Menu::Item{"Video", "assets/images/005-film-slate.png"},
        new Menu::Item{"Music", "assets/images/003-music.png"},
        new Menu::Item{"Walkie-Talkie", "assets/images/007-baby-monitor.png"},
        new SubmenuItem{"Apps", "assets/images/023-presents.png", &gui, {
            new Menu::Item{"Torchlight", "assets/images/004-flashlight.png"},
            new WidgetItem{"Paint", "assets/images/021-poo.png", new PixelEditor{&gui}},
            new Menu::Item{"Baby Monitor", "assets/images/006-baby-crib.png"},
        }},
    }};
    //Keyboard kb{};
    //gui.setWidget(&kb);
    gui.setMenu(& menu, 0);
    gui.loop();
}