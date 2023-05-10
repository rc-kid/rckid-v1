#include "raylib_cpp.h"

#include "platform/platform.h"

#include "rckid.h"
#include "window.h"
#include "menu.h"
//#include "window/carousel.h"
#include "pixel_editor.h"
#include "debug_view.h"
#include "keyboard.h"
#include "game_player.h"
#include "music_player.h"
#include "video_player.h"


/** Main RCKid app. 
 
    TODO start main loop and do some nice and efficient event loop for the main thread

 */
int main(int argc, char * argv[]) {

    Window window;
    Menu menu{{
        //new Menu::Item{"Games", "assets/images/001-game-controller.png"},
        new WidgetItem{"Games", "assets/images/001-game-controller.png", new GamePlayer{&window}},
        new WidgetItem{"Video", "assets/images/005-film-slate.png", new VideoPlayer{&window}},
        new WidgetItem{"Music", "assets/images/003-music.png", new MusicPlayer{&window}},
        //new JSONItem{"Music", "assets/images/003-music.png", "/rckid/music/folder.json", &window},
        new Menu::Item{"Remote", "assets/images/002-rc-car.png"},
        new Menu::Item{"Walkie-Talkie", "assets/images/007-baby-monitor.png"},
        new SubmenuItem{"Apps", "assets/images/023-presents.png", {
            new Menu::Item{"Torchlight", "assets/images/004-flashlight.png"},
            new WidgetItem{"Paint", "assets/images/021-poo.png", new PixelEditor{&window}},
            new Menu::Item{"Baby Monitor", "assets/images/006-baby-crib.png"},
        }},
    }};
    //Keyboard kb{&window};
    //window.setWidget(&kb);
    //DebugView db{&window};
    //window.setWidget(&db);
    window.setMenu(& menu, 0);
    window.loop();
}