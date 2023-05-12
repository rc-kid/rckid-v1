#pragma once

#include "utils/process.h"

#include "widget.h"
#include "window.h"

/** A video player, frontend to cvlc controlled the same way as retroarch is. 
 
    Play/Paue = btnA
    Dpad Left/Right = 10 seconds back/forward
    Left/Right = 1 minute back/forward

    TODO pause when blurred (such as home menu)
 */
class VideoPlayer : public Widget {
public:

    VideoPlayer(Window * window): Widget{window} {}

    bool fullscreen() const { return true; }

protected:

    void draw() override {
        if (player_.done())
            window()->back();
    }

    void onNavigationPush() override {
        player_ = utils::Process::capture(utils::Command{"cvlc", { "-I", "rc", "/rckid/videos/test.mkv"}});
        playing_ = true;
        window()->enableBackground(false);
    }

    void onNavigationPop() override {
        if (!player_.done())
            player_.kill();
        window()->enableBackground(true);
    }

    // play/pause
    void btnA(bool state) override {
        if (state) {
            if (playing_) {
                playing_ = false;
                player_.tx("pause\n", 6);
            } else {
                playing_ = true;
                player_.tx("play\n", 5);
            }
        }
    }

    // back 10 seconds
    void dpadLeft(bool state) override {
        /*
        if (state) {
            window()->rckid()->keyPress(RCKid::VLC_DELAY_10S, true);
            window()->rckid()->keyPress(RCKid::VLC_BACK, true);
        } else {
            window()->rckid()->keyPress(RCKid::VLC_BACK, false);
            window()->rckid()->keyPress(RCKid::VLC_DELAY_10S, false);
        }
        */
    }

    // forward 10 seconds
    void dpadRight(bool state) override {
        /*
        if (state) {
            window()->rckid()->keyPress(RCKid::VLC_DELAY_10S, true);
            window()->rckid()->keyPress(RCKid::VLC_FORWARD, true);
        } else {
            window()->rckid()->keyPress(RCKid::VLC_FORWARD, false);
            window()->rckid()->keyPress(RCKid::VLC_DELAY_10S, false);
        }
        */
    }

    // back one minute
    void btnL(bool state) override {
        /*
        if (state) {
            window()->rckid()->keyPress(RCKid::VLC_DELAY_1M, true);
            window()->rckid()->keyPress(RCKid::VLC_BACK, true);
        } else {
            window()->rckid()->keyPress(RCKid::VLC_BACK, false);
            window()->rckid()->keyPress(RCKid::VLC_DELAY_1M, false);
        }
        */
    }

    // forward one minute
    void btnR(bool state) override {
        /*
        if (state) {
            window()->rckid()->keyPress(RCKid::VLC_DELAY_1M, true);
            window()->rckid()->keyPress(RCKid::VLC_FORWARD, true);
        } else {
            window()->rckid()->keyPress(RCKid::VLC_FORWARD, false);
            window()->rckid()->keyPress(RCKid::VLC_DELAY_1M, false);
        }
        */
    }

    utils::Process player_;
    bool playing_ = false;
    
}; // Video