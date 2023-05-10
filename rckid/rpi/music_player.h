#pragma once

#include "utils/exec.h"


#include "window.h"

/** Audio player.

    The mp3 files are stored in the `/rckid/music` folder. In there, the `folder.json` describes the current folder which can be either   

    to get the mp3 cover album
 */
class MusicPlayer : public Widget {
public:

    MusicPlayer(Window * window): Widget{window} {}

    /** Extracts the artwork from given track into a specified filename. 

ffmpeg -i /mnt/c/Users/petam/Music/incomplete/c64/Christian\ Schüler\ -\ Druid\ version\ \(grassroots\ mix\).mp3 -an -vcodec copy cover.png
     * 
    */
    static bool extractArtwork(std::string const & track, std::string const & artworkPath) {
        return false;
    }

protected:

    void draw() override {
        UpdateMusicStream(track_);         
    }

    void onNavigationPush() override {
        track_ = LoadMusicStream("/rckid/xxx.mp3");
        PlayMusicStream(track_);
    }

    void onNavigationPop() override {
        StopMusicStream(track_);
        UnloadMusicStream(track_);
    }

    Music track_;
}; // Music
