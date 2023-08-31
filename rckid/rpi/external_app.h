#pragma once

#include "widget.h"
#include "window.h"
#include "utils/process.h"

/** Runs external application. 
 
    Turns off the RCKid's menu display as if running a game and enables the thumbstick, waiting forever (home button press), or until the selected command terminates. Useful for debugging purposes only. 

 */
class ExternalApp : public Widget {
public:
    ExternalApp() = default;
    ExternalApp(std::string const & path): appPath_{path} {}

protected:

    bool fullscreen() const { return true; }

    void tick() override {
        if (!appPath_.empty() && app_.done())
            window().back();
    }

    void draw(Canvas &) override {
        cancelRedraw();
    }

    void onNavigationPush() override { 
        window().enableBackground(false);
        rckid().setGamepadActive(true);
        if (!appPath_.empty())
            app_ = utils::Process::start(utils::Command{appPath_});
    }

    void onNavigationPop() override {
        if (!appPath_.empty() && !app_.done())
            app_.kill();
        window().enableBackground(true);
        rckid().setGamepadActive(false);
    }

    void btnHome(bool state) {
        if (state)
            window().back();
    }

    void btnB(bool state) { }

    void btnX(bool state) {
        if (state) {
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_ENABLE, true);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_SAVE_STATE, true);
            platform::cpu::delayMs(50);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_SAVE_STATE, false);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_ENABLE, false);
        }
    }

    std::string appPath_;
    utils::Process app_;

}; // ExternalApp