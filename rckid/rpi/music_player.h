#pragma once

#include <filesystem>

#include "utils/exec.h"
#include "utils/uuid.h"

#include "window.h"

/** Audio player.

    The mp3 files are stored in the `/rckid/music` folder. In there, the `folder.json` describes the current folder which can be either   

    to get the mp3 cover album

    Add the repeat forever/ repeat album menu
 */
class MusicPlayer : public Widget {
public:

    void play(json::Value const & music) {

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

class MusicBrowser : public DirSyncedCarousel {
public:

    MusicBrowser(std::string const & rootDir): DirSyncedCarousel{rootDir} { }

protected:
    
    std::optional<json::Value> getItemForFile(DirEntry const & entry) override {
        std::string ext = entry.path().extension();
        if (ext == ".mp3") {
            json::Value result{DirSyncedCarousel::getItemForFile(entry).value()};
            extractAndUpdateArtwork(entry.path(), result);
            return result;
        } else {
            return std::nullopt;
        }
    }

    void itemSelected(size_t index, json::Value & json) override {
        //player_.play(currentDir_ / json[MENU_FILENAME].value<std::string>());
    }


    /** Extracts the artwork from given track into a specified filename. 

        This is pretty costly function as it executes 2 extra processes per successful image extraction, but since it's only ever going to be called for a few files at a time when new music is added it probably does not matter. 
    */
    void extractAndUpdateArtwork(std::string const & track, json::Value & item) {
        std::string imgPath = RCKid::MUSIC_ARTWORK_DIR / newUuid(); 
        system(STR("ffmpeg -i \"" << track << "\" -an -vcodec copy \"" << imgPath << ".jpg\"").c_str());
        if (std::filesystem::exists(imgPath + ".jpg")) {
            system(STR("convert \"" << imgPath << ".jpg\" -resize 320x240 \"" << imgPath << ".png\"").c_str());
            std::filesystem::remove(imgPath + ".jpg");
            item[MENU_ICON] = imgPath + ".png";
        }
    }

    MusicPlayer player_;
}; 
