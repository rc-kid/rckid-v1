#pragma once

#include "widget.h"
#include "window.h"

#include "platform/platform.h"

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

    bool rendering = true;

    void onFocus() override {
        /*
        if (rendering) {
            window()->stopRendering();
            rendering = false;
        }
        */
    }

    void onBlur() override {
        /*
        if (!rendering) {
            window()->startRendering();
            rendering = true;
        }
        */
    }

    void draw() override {

    }

    void btnL(bool state) override {

    }

    void btnR(bool state) override {

    }

    void btnX(bool state) override {
        if (state && rendering) {
            std::cout << "Button X pressed" << std::endl;
            window()->stopRendering();
            rendering = false;
        }
    }

    void btnY(bool state) override {
        if (state && !rendering) {
            std::cout << "Button Y pressed" << std::endl;
            window()->startRendering();
            rendering = true;
        }
        
    }


}; // RetroArch