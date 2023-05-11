#pragma once

#include "widget.h"
#include "window.h"

#include "platform/platform.h"

#include "utils/process.h"


/** Retroarch client for a single game. 
 
    This is how to start a game:

    /opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/YOUR_CORE.so YOUR_ROM

    When running, the idea is that we'll use the gamepad's keyboard keys to trigger hotkeys in the app. 

    when loading the game we can decide which slot to run from start, we can also specify the save state via the configuration. 
 */
class GamePlayer : public Widget {
public:

    GamePlayer(Window * window): Widget{window} {}

    bool fullscreen() const { return true; }

protected:
    void draw() override {
        if (emulator_.done())
            window()->back();
    }

    void onNavigationPush() override {
        window()->rckid()->enableGamepad(true);
        emulator_ = utils::Process::start(utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch", { "-L", "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so", "/rckid/games/game.gbc"}});
    }

    void onNavigationPop() override {
        window()->rckid()->enableGamepad(false);
        if (!emulator_.done())
            emulator_.kill();
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

    utils::Process emulator_;
    

}; // RetroArch