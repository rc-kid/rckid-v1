#pragma once

#include "utils/process.h"

#include "widget.h"
#include "window.h"
#include "carousel_json.h"

/** A video player, frontend to cvlc controlled the same way as retroarch is. 
 
    Play/Paue = btnA
    Dpad Left/Right = 10 seconds back/forward
    Left/Right = 1 minute back/forward
 */
class VideoPlayer : public Widget {
public:

    bool fullscreen() const { return true; }

    void play(std::string const & path) {
        if (!player_.done())
            player_.kill();
        //std::string path = video["path"].value<std::string>();
        player_ = utils::Process::capture(utils::Command{"cvlc", { "-I", "rc", path}});
        playing_ = true;
        window().showWidget(this);
    }

protected:

    void draw() override {
        //DrawRectangle(0,0,320,240, ColorAlpha(BLACK, 0));
        if (player_.done())
            window().back();
        cancelRedraw();
    }

    void onNavigationPush() override {
    }

    void onNavigationPop() override {
        if (!player_.done())
            player_.kill();
        window().enableBackground(true);
    }

    void onFocus() {
        window().enableBackground(false);
        if (!playing_) {
            playing_ = true;
            player_.tx("play\n");
        }
    }

    void onBlur() {
        window().enableBackgroundDark(VIDEO_PLAYER_BACKGROUND_OPACITY);
        if (playing_) {
            playing_ = false;
            player_.tx("pause\n");
        }
    }

    // play/pause
    void btnA(bool state) override {
        if (state) {
            if (playing_) {
                playing_ = false;
                player_.tx("pause\n");
            } else {
                playing_ = true;
                player_.tx("play\n");
            }
        }
    }

    // back 10 seconds
    void dpadLeft(bool state) override {
        if (state)
            player_.tx("seek -10\n");
    }

    // forward 10 seconds
    void dpadRight(bool state) override {
        if (state)
            player_.tx("seek +10\n");
    }

    // back one minute
    void btnL(bool state) override {
        if (state)
            player_.tx("seek -60\n");
    }

    // forward one minute
    void btnR(bool state) override {
        if (state)
            player_.tx("seek +60\n");
    }

    utils::Process player_;
    bool playing_ = false;
    
}; // Video

class VideoBrowser : public DirSyncedCarousel {
public:
    VideoBrowser(std::string const & rootDir): DirSyncedCarousel{rootDir} { }

protected:
    
    std::optional<json::Value> getItemForFile(DirEntry const & entry) override {
        std::string ext = entry.path().extension();
        if (ext == ".mkv")
            return DirSyncedCarousel::getItemForFile(entry);
        else
            return std::nullopt;
    }

    void itemSelected(size_t index, json::Value & json) override {
        player_.play(currentDir_ / json[MENU_FILENAME].value<std::string>());
    }

    VideoPlayer player_;
}; 