#pragma once

#include "platform/platform.h"

#include "utils/process.h"
#include "utils/json.h"

#include "widget.h"
#include "window.h"
#include "carousel.h"
#include "carousel_json.h"
#include "gauge.h"



/** Retroarch client for a single game. 
 
    This is how to start a game:

    /opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/YOUR_CORE.so YOUR_ROM

    When running, the idea is that we'll use the gamepad's keyboard keys to trigger hotkeys in the app. 

    when loading the game we can decide which slot to run from start, we can also specify the save state via the configuration. 

    // game menu is: save game, load game, screenshot, exit game


 */
class GamePlayer : public Widget {
public:
    static inline std::string const GAME_EMULATOR{"emulator"};
    static inline std::string const GAME_LRCORE{"lrcore"};
    static inline std::string const GAME_DOSBOX_CONFIG{"dosbox-config"};

    static inline std::string const EMULATOR_RETROARCH{"retroarch"};
    static inline std::string const EMULATOR_DOSBOX{"dosbox"};

    GamePlayer(): 
        gameMenu_{new Carousel::Menu{"", "", {
            new Carousel::Item{"Volume", "assets/images/010-high-volume.png", [](){
                static Gauge gauge{"assets/images/010-high-volume.png", 0, 100, 10,
                    [](int value){
                        rckid().setVolume(value);
                    },
                    [](Gauge * g) {
                        g->setValue(rckid().volume());
                    }
                };
                window().showWidget(&gauge);
            }},
            new Carousel::Item{"Save", "assets/images/071-diskette.png", [](){
                // TODO
            }},
            new Carousel::Item{"Load", "assets/images/069-open.png", [](){
                // TODO
            }},
            new Carousel::Item{"Screenshot", "assets/images/063-screenshot.png", [](){
                // TODO
            }},
            new Carousel::Item{"Resume", "assets/images/065-play.png", [](){
                window().back();
            }},
            new Carousel::Item{"Exit", "assets/images/064-stop.png", [](){
                window().back(2);
            }}
        }}} {
        }

    bool fullscreen() const { return true; }

    void play(std::filesystem::path dir,  json::Value & game) {
        std::string emulator = game[GAME_EMULATOR].value<std::string>();
        utils::Command cmd;
        if (emulator == EMULATOR_RETROARCH) {
            std::string path = dir / game[DirSyncedCarousel::MENU_FILENAME].value<std::string>();
            std::string core = game[GAME_LRCORE].value<std::string>();
            cmd = utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch", { "--config", "/home/pi/rckid/retroarch/retroarch.cfg", "-L", core.c_str(), path.c_str()}};
        } else if (emulator == EMULATOR_DOSBOX) {
            std::string config = game[GAME_DOSBOX_CONFIG].value<std::string>();
            // /opt/retropie/emulators/dosbox/bin/dosbox -v -conf /rckid/games/dos/WackyWhe/dosbox.conf
            cmd = utils::Command{"/opt/retropie/emulators/dosbox/bin/dosbox", { "-conf", config.c_str()}};
        }
        // there is no retropie on my dev machine so playing with glxgears instead
#if (defined ARCH_RPI)
        emulator_ = utils::Process::start(cmd);
        //emulator_ = utils::Process::start(utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch"});
#else
        emulator_ = utils::Process::start(utils::Command{"glxgears"});
#endif
        window().showWidget(this);
    }

protected:
    void tick() override {
        if (emulator_.done())
            window().back();
    }

    void draw(Canvas &) override {
        cancelRedraw();
    }

    // handled by the play method above
    //void onNavigationPush() override { }

    void onNavigationPop() override {
        if (!emulator_.done())
            emulator_.kill();
        window().enableBackground(true);
    }

    void onFocus() {
        window().enableBackground(false);
        if (! emulator_.done()) {
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_ENABLE, true);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_PAUSE, true);
            platform::cpu::delayMs(50);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_PAUSE, false);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_ENABLE, false);
        }
        rckid().setGamepadActive(true);
    }

    void onBlur() {
        window().enableBackgroundDark(GAME_PLAYER_BACKGROUND_OPACITY);
        if (! emulator_.done()) {
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_ENABLE, true);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_PAUSE, true);
            platform::cpu::delayMs(50);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_PAUSE, false);
            rckid().keyPress(RCKid::RETROARCH_HOTKEY_ENABLE, false);
        }
        rckid().setGamepadActive(false);
    }

    // don't do anything here, prevents the back behavior
    void btnB(bool state) { }


    void btnHome(bool state) {
        if (state)
            window().showWidget(& gameMenu_);
    }

    utils::Process emulator_;

    Carousel gameMenu_;
    

}; // GamePlayer

class GameBrowser : public DirSyncedCarousel {
public:
    GameBrowser(std::string const & rootDir): DirSyncedCarousel{rootDir, "assets/icons/ghost.png", "assets/icons/arcade.png"} {}

protected:

    std::optional<json::Value> getItemForFile(DirEntry const & entry) override {
        std::string ext = entry.path().extension();
        if (ext == ".gb")
            return createGameBoyProfile(DirSyncedCarousel::getItemForFile(entry).value());
        else if (ext == ".gbc")
            return createGameBoyColorProfile(DirSyncedCarousel::getItemForFile(entry).value());
        else if (ext == ".gba")
            return createGameBoyAdvanceProfile(DirSyncedCarousel::getItemForFile(entry).value());
        else if (ext == ".md")
            return createSegaGenesisProfile(DirSyncedCarousel::getItemForFile(entry).value());
        else if (ext == ".n64")
            return createNintendo64Profile(DirSyncedCarousel::getItemForFile(entry).value());
        else if (ext == ".sfc")
            return createSuperNintendoProfile(DirSyncedCarousel::getItemForFile(entry).value());
        else
            return std::nullopt;
    }

    json::Value && createRetroarchProfile(json::Value && item, std::string core) {
        item[GamePlayer::GAME_EMULATOR] = GamePlayer::EMULATOR_RETROARCH;
        item[GamePlayer::GAME_LRCORE] = core;
        return std::move(item);
    }

    json::Value && createGameBoyProfile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so");
    }

    json::Value && createGameBoyColorProfile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so");
    }

    json::Value && createGameBoyAdvanceProfile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so");
    }

    json::Value && createSegaGenesisProfile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "/opt/retropie/libretrocores/lr-genesis-plus-gx/genesis_plus_gx_libretro.so");
    }

    json::Value && createNintendo64Profile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "/opt/retropie/libretrocores/lr-mupen64plus/mupen64plus_libretro.so");
    }

    json::Value && createSuperNintendoProfile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "/opt/retropie/libretrocores/lr-snes9x2005/snes9x2005_libretro.so");
    }

    void itemSelected(size_t index, json::Value & json) override {
        player_.play(currentDir_, json);
    }

    GamePlayer player_;
    
};