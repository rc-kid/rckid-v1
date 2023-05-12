#pragma once

#include "widget.h"
#include "window.h"
#include "menu.h"

#include "platform/platform.h"

#include "utils/process.h"


/** Retroarch client for a single game. 
 
    This is how to start a game:

    /opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/YOUR_CORE.so YOUR_ROM

    When running, the idea is that we'll use the gamepad's keyboard keys to trigger hotkeys in the app. 

    when loading the game we can decide which slot to run from start, we can also specify the save state via the configuration. 

    // game menu is: save game, load game, screenshot, exit game

 */
class GamePlayer : public Widget {
public:

    GamePlayer(Window * window): 
        Widget{window},
        gameMenu_{{
            new ActionItem{"Resume", "assets/images/066-play.png", [window](){
                window->back();
            }},
            new Menu::Item{"Save", "assets/images/072-diskette.png"},
            new Menu::Item{"Load", "assets/images/070-open.png"},
            new Menu::Item{"Screenshot", "assets/images/064-screenshot.png"},
            new ActionItem{"Exit", "assets/images/065-stop.png", [window](){
                window->back(2);
            }},
        }}  {}

    bool fullscreen() const { return true; }

protected:
    void draw() override {
        if (emulator_.done())
            window()->back();
    }

    void onNavigationPush() override {
        window()->rckid()->enableGamepad(true);
        // there is no retropie on my dev machine so playing with glxgears instead
#if (defined ARCH_RPI)
        emulator_ = utils::Process::start(utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch", { "--config", "/home/pi/rckid/retroarch.cfg", "-L", "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so", "/rckid/games/game.gbc"}});
        //emulator_ = utils::Process::start(utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch"});
#else
        emulator_ = utils::Process::start(utils::Command{"glxgears"});
#endif
        window()->enableBackground(false);
    }

    void onNavigationPop() override {
        window()->rckid()->enableGamepad(false);
        if (!emulator_.done())
            emulator_.kill();
        window()->enableBackground(true);
    }

    void onFocus() {
        if (! emulator_.done()) {
            window()->rckid()->keyPress(RCKid::RETROARCH_PAUSE, true);
            window()->rckid()->keyPress(RCKid::RETROARCH_PAUSE, false);
        }

    }

    void onBlur() {
        if (! emulator_.done()) {
            window()->rckid()->keyPress(RCKid::RETROARCH_PAUSE, true);
            window()->rckid()->keyPress(RCKid::RETROARCH_PAUSE, false);
        }
    }

    // don't do anything here, prevents the back behavior
    void btnB(bool state) { }


    void btnHome(bool state) {
        if (state)
            window()->setMenu(& gameMenu_, 0);
    }

    utils::Process emulator_;

    Menu gameMenu_;
    

}; // RetroArch