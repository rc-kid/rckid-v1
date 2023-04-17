#pragma once

#include "window.h"


/** Audio player.

    The mp3 files are stored in the `/rckid/music` folder. In there, the `folder.json` describes the current folder which can be either   
 */
class MusicPlayer : public Widget {
public:

    MusicPlayer(Window * window): Widget{window} {}

protected:

    void draw() override {
    }

    void onFocus() override {
        track_ = LoadMusicStream("/mnt/c/Users/petam/Music/incomplete/Tangerine Kitty/Tangerine Kitty - Dumb Ways To Die.mp3");
        PlayMusicStream(track_);
    }

    void onBlur() override {
        UnloadMusicStream(track_);
    }

    Music track_;
}; // Music
