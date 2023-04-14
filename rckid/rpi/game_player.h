#pragma once

#include "widget.h"
#include "gui.h"

#include "platform/platform.h"

/** Retroarch client for a single game. 
 
    This is how to start a game:

    /opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/YOUR_CORE.so YOUR_ROM

    When running, the idea is that we'll use the gamepad's keyboard keys to trigger hotkeys in the app. 

    when loading the game we can decide which slot to run from start, we can also specify the save state via the configuration. 
 */
class GamePlayer : public Widget {
public:

    GamePlayer(GUI * gui): Widget{gui} {}

   bool fullscreen() const { return true; }

protected:

    void draw() override {

    }

    void btnL(bool state) override {

    }

    void btnR(bool state) override {

    }

    void btnA(bool state) override {
        static bool rendering = true;
        if (state) {
            if (rendering)
                gui()->stopRendering();
            else
                gui()->startRendering();
            rendering = ! rendering;
        }
    }

    void btnB(bool state) override {

    }

    void btnX(bool state) override {

    }

    void btnY(bool state) override {

    }

}; // RetroArch