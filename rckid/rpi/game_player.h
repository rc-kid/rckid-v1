#pragma once

#include <filesystem>

#include "platform/platform.h"

#include "utils/process.h"
#include "utils/json.h"

#include "widget.h"
#include "window.h"
#include "carousel.h"
#include "carousel_json.h"
#include "gauge.h"
#include "wait.h"



/** Retroarch client for a single game. 
 
    This is how to start a game:

    /opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/YOUR_CORE.so YOUR_ROM

    When running, the idea is that we'll use the gamepad's keyboard keys to trigger hotkeys in the app. 

    when loading the game we can decide which slot to run from start, we can also specify the save state via the configuration. 

    // game menu is: save game, load game, screenshot, exit game

    Note: in order for the game to allow saving, the libretro core must be copied into ~/.config/retroarch/cores. 

 */
class GamePlayer : public Widget {
public:
    static inline std::string const GAME_EMULATOR{"emulator"};
    static inline std::string const GAME_LRCORE{"lrcore"};
    static inline std::string const GAME_DOSBOX_CONFIG{"dosbox-config"};

    static inline std::string const EMULATOR_RETROARCH{"retroarch"};
    static inline std::string const EMULATOR_DOSBOX{"dosbox"};

    bool fullscreen() const { return true; }

    void play(std::filesystem::path dir,  json::Value & game) {
        std::string emulator = game[GAME_EMULATOR].value<std::string>();
        utils::Command cmd;
        if (emulator == EMULATOR_RETROARCH) {
            /*
            std::string path = dir / game[DirSyncedCarousel::MENU_FILENAME].value<std::string>();
            std::string core = game[GAME_LRCORE].value<std::string>();
            cmd = utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch", { "--config", "/home/pi/rckid/retroarch/retroarch.cfg", "-L", core.c_str(), path.c_str()}};
            //cmd = utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch", { "--config", "/home/pi/rckid/retroarch/retroarch.cfg"}};
            */
            std::string path = dir / game[DirSyncedCarousel::MENU_FILENAME].value<std::string>();
            std::string core = game[GAME_LRCORE].value<std::string>();
            cmd = utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch", { "--config", "/home/pi/rckid/retroarch/retroarch.cfg", "-L", core.c_str(), path.c_str()}, "/home/pi/.config/retroarch/cores"};
            //cmd = utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch", { "--config", "/home/pi/rckid/retroarch/retroarch.cfg"}};
            std::cout << cmd.command();
            for (auto i : cmd.args()) 
                std::cout << i << std::endl;
            std::cout << cmd.workingDirectory() << std::endl;

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
        // determine the save and snapshot directories to be used for the game. This is the position of the game in the /rckid/games folder 
        game_ = dir / game[DirSyncedCarousel::MENU_FILENAME].value<std::string>();
        TraceLog(LOG_DEBUG, STR("Running game " << game_));
        auto p = std::filesystem::relative(game_.parent_path() / game_.stem(), "/rckid/games");
        snapshotDir_ = std::filesystem::path{"/rckid/.rckid/snapshots/games/"} / p;
        saveDir_ = std::filesystem::path{"/rckid/.rckid/saves/"} / p;
        TraceLog(LOG_DEBUG, STR("  snapshot dir: " << snapshotDir_));        
        TraceLog(LOG_DEBUG, STR("  save dir: " << saveDir_));        
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

    void onNavigationPush() override { 
        rckid().enableHeartsCounter(true);
    }

    void onNavigationPop() override {
        if (!emulator_.done())
            emulator_.kill();
        window().enableBackground(true);
        rckid().enableHeartsCounter(false);
    }

    void onFocus() {
        window().enableBackground(false);
        if (! emulator_.done())
            retroarchHotkey(RCKid::RETROARCH_HOTKEY_PAUSE);
        rckid().setGamepadActive(true);
    }

    void onBlur() {
        window().enableBackgroundDark(GAME_PLAYER_BACKGROUND_OPACITY);
        if (! emulator_.done())
            retroarchHotkey(RCKid::RETROARCH_HOTKEY_PAUSE);
        rckid().setGamepadActive(false);
    }

    // don't do anything here, prevents the back behavior
    void btnB(bool state) { }


    void btnHome(bool state) {
        if (state) {
            gameMenu_ = Carousel{initializeGameMenu()};
            window().showWidget(& gameMenu_);
        }
    }

    void retroarchHotkey(unsigned int hkey) {
        rckid().keyPress(RCKid::RETROARCH_HOTKEY_ENABLE, true);
        rckid().keyPress(hkey, true);
        platform::cpu::delayMs(50);
        rckid().keyPress(hkey, false);
        rckid().keyPress(RCKid::RETROARCH_HOTKEY_ENABLE, false);
    }

    /** Checks the tmp folder for corresponding screenshot name and returns its full path, or empty string when no screenshot found. 
     */
    std::filesystem::path getTmpScreenshotName() {
        using namespace std::filesystem;
        std::string namePrefix = game_.stem();
        for (auto const & e : directory_iterator(SCREENSHOT_TMP_DIRECTORY)) {
            if (e.is_regular_file() && e.path().extension() == ".png") {
                if (str::startsWith(e.path().stem(), namePrefix.c_str()))
                    return e.path();
            } 
        }   
        return std::filesystem::path{}; 
    }

    Carousel::Menu * initializeGameMenu() {
        Carousel::Menu * m = new Carousel::Menu{};
        // single save state
        m->append(new Carousel::Item{"Save", "assets/images/071-diskette.png", [this](){
                retroarchHotkey(RCKid::RETROARCH_HOTKEY_SAVE_STATE);
                retroarchHotkey(RCKid::RETROARCH_HOTKEY_SCREENSHOT);
                wait_.showDelay(1000, [this](Wait *) {
                    std::filesystem::create_directories(saveDir_);
                    std::filesystem::path tmpName{getTmpScreenshotName()};
                    if (!tmpName.empty()) {
                        // move the screenshot and the save state
                        std::filesystem::rename(tmpName, saveDir_ / "user.png");
                        // move the saved state
                        auto p = game_;
                        p.replace_extension("state");
                        std::filesystem::rename(p, saveDir_ / "user.state");
                        return true;
                    } else {
                        return false;
                    }
                });
                rckid().rumbler(128, 10);
            }}
        );
        // if there is user saved state, add load menu
        if (std::filesystem::exists(saveDir_ / "user.state")) {
            m->append(new Carousel::Item{"Load", saveDir_ / "user.png", [this]() {
                auto p = game_;
                p.replace_extension("state");
                if (std::filesystem::exists(p))
                    std::filesystem::remove(p);
                std::filesystem::copy_file(saveDir_ / "user.state", p);
                retroarchHotkey(RCKid::RETROARCH_HOTKEY_LOAD_STATE);
                rckid().rumbler(128,10);
                wait_.showDelay(1000, [this, p](Wait *){
                    remove(p);
                    window().back();
                    return true;
                });
            }});
        }
        // if there is latest save, add latest menu
        if (std::filesystem::exists(saveDir_ / "latest.state")) {
            m->append(new Carousel::Item{"Latest", saveDir_ / "latest.png", [this]() {
                auto p = game_;
                p.replace_extension("state");
                if (std::filesystem::exists(p))
                    std::filesystem::remove(p);
                std::filesystem::copy_file(saveDir_ / "latest.state", p);
                retroarchHotkey(RCKid::RETROARCH_HOTKEY_LOAD_STATE);
                rckid().rumbler(128,10);
                wait_.showDelay(1000, [this, p](Wait *){
                    remove(p);
                    window().back();
                    return true;
                });
            }});
        }
        m->append(new Carousel::Item{"Screenshot", "assets/images/063-screenshot.png", [this](){
                // instruct retroarch to create the screenshot
                retroarchHotkey(RCKid::RETROARCH_HOTKEY_SCREENSHOT);
                rckid().rumbler(128,10);
                // wait for 2 seconds
                wait_.showDelay(1000, [this](Wait *) {
                    // since screenshots are generated to a tmp directory and with random-ish names, check the dir contents now and move it to the target folder
                    std::filesystem::create_directories(snapshotDir_);
                    std::filesystem::path tmpName{getTmpScreenshotName()};
                    if (!tmpName.empty()) {
                        std::filesystem::rename(tmpName, snapshotDir_ / tmpName.filename());
                        return true;
                    } else {
                        return false;
                    }
                });
            }}
        );
        m->append(new Carousel::Item{"Resume", "assets/images/065-play.png", [](){
                window().back();
            }}
        );
        m->append(new Carousel::Item{"Exit", "assets/images/064-stop.png", [this](){
                retroarchHotkey(RCKid::RETROARCH_HOTKEY_SAVE_STATE);
                retroarchHotkey(RCKid::RETROARCH_HOTKEY_SCREENSHOT);
                wait_.showDelay(1000, [this](Wait *) {
                    std::filesystem::create_directories(saveDir_);
                    std::filesystem::path tmpName{getTmpScreenshotName()};
                    if (!tmpName.empty()) {
                        // move the screenshot and the save state
                        std::filesystem::rename(tmpName, saveDir_ / "latest.png");
                        // move the saved state
                        auto p = game_;
                        p.replace_extension("state");
                        std::filesystem::rename(p, saveDir_ / "latest.state");
                    }
                    window().back(2);
                    return true;
                });
           }}
        );
        return m;
    }

    utils::Process emulator_;

    Carousel gameMenu_{nullptr};
    Wait wait_;

    //std::string name_;
    //std::filesystem::path game_;
    std::filesystem::path game_;
    std::filesystem::path snapshotDir_;
    std::filesystem::path saveDir_;

}; // GamePlayer

class GameBrowser : public DirSyncedCarousel {
public:
    GameBrowser(std::string const & rootDir): DirSyncedCarousel{rootDir, "assets/icons/ghost.png", "assets/icons/arcade.png"} {}

protected:

    /*
    void onNavigationPush() override { 
        DirSyncedCarousel::onNavigationPush();
        rckid().enableHeartsCounter(true);
    }

    void onNavigationPop() override {
        DirSyncedCarousel::onNavigationPop();
        rckid().enableHeartsCounter(false);
    }
    */


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
        return createRetroarchProfile(std::move(item), "mgba");
    }

    json::Value && createGameBoyColorProfile(json::Value && item) {
        //return createRetroarchProfile(std::move(item), "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so");
        return createRetroarchProfile(std::move(item), "mgba");
    }

    json::Value && createGameBoyAdvanceProfile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "mgba");
    }

    json::Value && createSegaGenesisProfile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "genesis_plus_gx.so");
    }

    json::Value && createNintendo64Profile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "mupen64plus");
    }

    json::Value && createSuperNintendoProfile(json::Value && item) {
        return createRetroarchProfile(std::move(item), "snes9x2005");
    }

    void itemSelected(size_t index, json::Value & json) override {
        player_.play(currentDir_, json);
    }

    GamePlayer player_;
};

