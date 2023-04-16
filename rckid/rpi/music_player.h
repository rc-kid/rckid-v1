#pragma once

#include "window.h"


/** Audio player. 
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
