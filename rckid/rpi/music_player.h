#pragma once

#include <filesystem>
#include <mutex>
#include <thread>

#include "utils/exec.h"
#include "utils/uuid.h"

#include "window.h"

/** Music player runs in its own thread, which allows us to play the music without issues even if the renderer gets an FPS drop as well as to keep playing music even when the music menu is left. 
 */
class MusicPlayer {
public:

    MusicPlayer() {
        using namespace platform;
        t_ = std::thread([this]() {
            while (tick())
                cpu::delayMs(5);
        });
    }

    bool isPlaying() { std::lock_guard g{m_}; return playing_; }

    /** Plays the given playlist, starting with the item at given index. 
     */
    void setPlaylist(json::Value * playlist, std::filesystem::path const & playlistDir) {
        stop();
        playlist_ = playlist;
        playlistDir_ = playlistDir;
    }

    void play(size_t index) {
        json::Value const & item = (*playlist_)[index];
        std::string f = playlistDir_ / item[DirSyncedCarousel::MENU_FILENAME].value<std::string>();
        std::lock_guard g{m_};
        track_ = LoadMusicStream(f.c_str());
        track_.looping = false;
        PlayMusicStream(track_);
        trackLength_ = GetMusicTimeLength(track_);
        playing_ = true;
    }

    float elapsed() {
        std::lock_guard g{m_};
        return GetMusicTimePlayed(track_);
    }

    float trackLength() {
        return trackLength_;
    }

    void loopSingle(bool value) {
        std::lock_guard g{m_};
        track_.looping = value;
    }

    void pause(bool value) {
        std::lock_guard g{m_};
        playing_ = ! value;
    }

    void togglePause() {
        std::lock_guard g{m_};
        playing_ = ! playing_;
    }

    void stop() {
        std::lock_guard g{m_};
        playing_ = false;
        if (playlist_ != nullptr)
            UnloadMusicStream(track_);
        playlist_ = nullptr;
    }
private:

    void playFile(size_t index) {
    }

    bool tick() {
        std::lock_guard g{m_};
        if (!playing_)
            return true;
        if (IsMusicStreamPlaying(track_))
            UpdateMusicStream(track_);
        else 
            playing_ = false;
        return true;
    }


    json::Value * playlist_ = nullptr;
    std::filesystem::path playlistDir_;
    float trackLength_;

    std::mutex m_;
    std::thread t_;

    bool playing_ = false;
    Music track_;


}; 


class MusicBrowser : public DirSyncedCarousel {
public:

    MusicBrowser(std::string const & rootDir): 
        DirSyncedCarousel{rootDir},
        play_{LoadTexture("assets/icons/play-64.png")},
        pause_{LoadTexture("assets/icons/pause-64.png")},
        repeat_{LoadTexture("assets/icons/repeat-32.png")},
        gauge_{LoadTexture("assets/gauge.png")} {
    }

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
        browsing_ = false;
        requestRedraw();
        player_.setPlaylist(& currentJson()[MENU_SUBITEMS], currentDir_);
        player_.play(index);
        player_.loopSingle(repeatSingleTrack_);
        setShowTitle(false);

        //track_ = LoadMusicStream(currentDir_ / json[MENU_FILENAME].value<std::string>());
        //PlayMusicStream(track_);
        //trackLength_ = GetMusicTimeLength(track_);
    }


    /** Extracts the artwork from given track into a specified filename. 

        This is pretty costly function as it executes 2 extra processes per successful image extraction, but since it's only ever going to be called for a few files at a time when new music is added it probably does not matter. We take the album art and convert it to 320x240 max so that it fits the screen nicely. 
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

    void draw() override {
        DirSyncedCarousel::draw();
        // if we are playing, display the extra 
        if (! browsing_) {
            BeginBlendMode(BLEND_ALPHA);
            int startY = 146; // 83
            DrawRectangle(0, startY, 320, 74, ColorAlpha(BLACK, 0.5));
            if (player_.isPlaying()) {
                DrawTexture(play_, 5, startY + 5, WHITE);
                requestRedraw();   
            } else {
                DrawTexture(pause_, 5, startY + 5, WHITE);
            }    
            if (repeatSingleTrack_)
                DrawTexture(repeat_, 45, 45, WHITE);
            float elapsed = player_.elapsed();
            float total = player_.trackLength();            
            std::string elapsedStr = toHMS(static_cast<int>(elapsed));
            DrawTexture(gauge_, 75, startY + 10, DARKGRAY);
            BeginScissorMode(75, startY + 10, 240 * elapsed / total, 10);
            DrawTexture(gauge_, 75, startY + 10, BLUE);
            EndScissorMode();
            //window().drawProgressBar(75, 43, 240, 10, elapsed / trackLength_, DARKGRAY, BLUE);
            Font f = window().headerFont();

            std::string remainingStr = toHMS(static_cast<int>(total - elapsed));
            int remainingWidth = MeasureTextEx(window().headerFont(), remainingStr.c_str(), window().headerFont().baseSize, 1.0).x;

            DrawTextEx(f, elapsedStr.c_str(), 75, startY + 25, f.baseSize, 1.0, WHITE);
            DrawTextEx(f, remainingStr.c_str(), 315 - remainingWidth, startY + 25, f.baseSize, 1.0, WHITE);

            DrawTextEx(f, currentTitle().c_str(), 75, startY + 45, f.baseSize, 1.0, WHITE);
        }
    }

    void btnA(bool state) override {
        if (browsing_) {
            DirSyncedCarousel::btnA(state);
        } else if (state) {
            player_.togglePause();
        }
    }

    void btnB(bool state) override {
        if (browsing_) {
            DirSyncedCarousel::btnB(state);
        } else if (state) {
            player_.stop();
            browsing_ = true;
            setShowTitle(true);
            requestRedraw();
        }
    }

    void btnX(bool state) override {
        if (browsing_) {
            DirSyncedCarousel::btnX(state);
        } else if (state) {
            repeatSingleTrack_ = ! repeatSingleTrack_;
            player_.loopSingle(repeatSingleTrack_);
            requestRedraw();
        }
    }

    bool browsing_ = true;
    bool repeatSingleTrack_ = false;

    MusicPlayer player_;
    
    Texture2D play_;
    Texture2D pause_;
    Texture2D repeat_;
    Texture2D gauge_;

}; 
