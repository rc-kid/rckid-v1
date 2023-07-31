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

    bool paused() { std::lock_guard g{m_}; return paused_; }

    bool done() { std::lock_guard g{m_}; return loaded_ == false; }

    void play(std::string const & filename) {
        // first stop
        stop();
        // then set the new track
        std::lock_guard g{m_};
        track_ = LoadMusicStream(filename.c_str());
        track_.looping = false;
        PlayMusicStream(track_);
        trackLength_ = GetMusicTimeLength(track_);
        // enter pause mode if the audio device is not ready
        paused_ = ! IsAudioDeviceReady();
        loaded_ = true;
    }

    float elapsed() {
        std::lock_guard g{m_};
        return GetMusicTimePlayed(track_);
    }

    float trackLength() {
        return trackLength_;
    }

    void pause(bool value) {
        std::lock_guard g{m_};
        paused_ = value;
    }

    void togglePause() {
        std::lock_guard g{m_};
        paused_ = ! paused_;
    }

    void stop() {
        std::lock_guard g{m_};
        if (loaded_) {
            UnloadMusicStream(track_);
            loaded_ = false;
        } 
    }
private:

    bool tick() {
        std::lock_guard g{m_};
        if (paused_)
            return true;
        if (IsMusicStreamPlaying(track_)) {
            UpdateMusicStream(track_);
        } else if (loaded_) { 
            UnloadMusicStream(track_);
            loaded_ = false;
            paused_ = true;
        }
        return true;
    }

    json::Value * playlist_ = nullptr;
    std::filesystem::path playlistDir_;
    float trackLength_;

    std::mutex m_;
    std::thread t_;

    bool paused_ = true;
    bool loaded_ = false;
    Music track_;


}; 


class MusicBrowser : public DirSyncedCarousel {
public:

    MusicBrowser(std::string const & rootDir): 
        DirSyncedCarousel{rootDir, "assets/icons/musical-note.png", "assets/icons/music.png"},
        play_{"assets/icons/play-64.png"},
        pause_{"assets/icons/pause-64.png"},
        repeat_{"assets/icons/repeat-32.png"},
        gauge_{"assets/gauge-music.png"} {
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
        playCurrentTrack();
        /*
        requestRedraw();
        player_.play(currentDir_ / json[MENU_FILENAME].value<std::string>());
        setShowTitle(false);
        titleWidth_ = 0;
        titleScroller_.reset();
        setFooterHints();
        */

        //track_ = LoadMusicStream(currentDir_ / json[MENU_FILENAME].value<std::string>());
        //PlayMusicStream(track_);
        //trackLength_ = GetMusicTimeLength(track_);
    }

    bool playCurrentTrack() {
        json::Value & json = currentJson();
        if (! json.containsKey(MENU_FILENAME))
            return false;
        player_.play(currentDir_ / json[MENU_FILENAME].value<std::string>());
        setShowTitle(false);
        Canvas & c = window().canvas();
        titleWidth_ = c.textWidth(currentTitle(), c.defaultFont());
        setFooterHints();
        requestRedraw();
        return true;
    }

    void setFooterHints() override {
        if (browsing_) {
            DirSyncedCarousel::setFooterHints();
        } else {
            window().clearFooter();
            window().addFooterItem(FooterItem::B(" "));
            window().addFooterItem(FooterItem::A("  "));
            window().addFooterItem(FooterItem::X(" "));
        }
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
        Canvas & c = window().canvas();
        DirSyncedCarousel::draw();
        // if we are playing, display the extra 
        if (! browsing_) {
            // se if we should move to next song / start playing again
            if (player_.done()) {
                size_t i = currentNumItems();
                while (i > 0) {
                    if (!repeatSingleTrack_)
                        goToNext();
                    if (playCurrentTrack())
                        break;
                    --i;
                }
                if (i == 0) {
                    player_.stop();
                    browsing_ = true;
                }
            }
            // display the player's UI
            BeginBlendMode(BLEND_ALPHA);
            int startY = 146; // 83
            DrawRectangle(0, startY, 320, 74, ColorAlpha(BLACK, 0.5));
            if (player_.paused()) {
                c.drawTexture(5, startY + 5, pause_);
            } else {
                c.drawTexture(5, startY + 5, play_);
                requestRedraw();   
            }    
            if (repeatSingleTrack_)
                c.drawTexture(40, startY + 40, repeat_);
            float elapsed = player_.elapsed();
            float total = player_.trackLength();            
            std::string elapsedStr = toHMS(static_cast<int>(elapsed));
            c.drawTexture(75, startY + 10, gauge_, DARKGRAY);
            BeginScissorMode(75, startY + 10, 240 * elapsed / total, 10);
            c.drawTexture(75, startY + 10, gauge_, BLUE);
            EndScissorMode();
            //window().drawProgressBar(75, 43, 240, 10, elapsed / trackLength_, DARKGRAY, BLUE);
            c.setDefaultFont();
            c.setFg(WHITE);


            std::string remainingStr = toHMS(static_cast<int>(total - elapsed));
            int remainingWidth = c.textWidth(remainingStr);

            c.drawText(75, startY + 25, elapsedStr, WHITE);
            c.drawText(315 - remainingWidth, startY + 25, remainingStr, WHITE);

            if (c.drawScrolledText(75, startY+45, 240, currentTitle(), titleWidth_, WHITE))
                requestRedraw();
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
            setFooterHints();
        }
    }

    void btnX(bool state) override {
        if (browsing_) {
            DirSyncedCarousel::btnX(state);
        } else if (state) {
            repeatSingleTrack_ = ! repeatSingleTrack_;
            requestRedraw();
        }
    }

    void dpadLeft(bool state) override {
        DirSyncedCarousel::dpadLeft(state);
        if (state && !browsing_) {
            if (!playCurrentTrack()) {
                player_.stop();
                browsing_ = true;
            }
        }
    }

    void dpadRight(bool state) override {
        DirSyncedCarousel::dpadRight(state);
        if (state && !browsing_) {
            if (!playCurrentTrack()) {
                player_.stop();
                browsing_ = true;
            }
        }
    }

    bool browsing_ = true;
    bool repeatSingleTrack_ = false;

    MusicPlayer player_;
    int titleWidth_ = 0;
    
    Canvas::Texture play_;
    Canvas::Texture pause_;
    Canvas::Texture repeat_;
    Canvas::Texture gauge_;

}; 
