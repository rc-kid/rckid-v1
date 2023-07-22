#pragma once

#include "widget.h"
#include "window.h"
#include "menu.h"

#include "platform/platform.h"

#include "utils/process.h"
#include "utils/json.h"


/** Retroarch client for a single game. 
 
    This is how to start a game:

    /opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/YOUR_CORE.so YOUR_ROM

    When running, the idea is that we'll use the gamepad's keyboard keys to trigger hotkeys in the app. 

    when loading the game we can decide which slot to run from start, we can also specify the save state via the configuration. 

    // game menu is: save game, load game, screenshot, exit game


 */
class GamePlayer : public Widget {
public:

    GamePlayer(): 
        gameMenu_{{
            new ActionItem{"Resume", "assets/images/066-play.png", [](){
                window().back();
            }},
            new Menu::Item{"Save", "assets/images/072-diskette.png"},
            new Menu::Item{"Load", "assets/images/070-open.png"},
            new Menu::Item{"Screenshot", "assets/images/064-screenshot.png"},
            new ActionItem{"Exit", "assets/images/065-stop.png", [](){
                window().back(2);
            }},
        }}  {}

    bool fullscreen() const { return true; }

    void play(json::Value const & game) {
        std::string emulator = game.containsKey("emulator") ? game["emulator"].value<std::string>() : "retroarch";
        utils::Command cmd;
        if (emulator == "retroarch") {
            std::string path = game["path"].value<std::string>();
            std::string core = (game.containsKey("lrcore")) ? game["lrcore"].value<std::string>() : libretroCoreForPath(path);
            cmd = utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch", { "--config", "/home/pi/rckid/retroarch/retroarch.cfg", "-L", core.c_str(), path.c_str()}};
            //emulator_ = utils::Process::start(utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch"});
        } else if (emulator == "dosbox") {
            std::string config = game["config"].value<std::string>();
            // /opt/retropie/emulators/dosbox/bin/dosbox -v -conf /rckid/games/dos/WackyWhe/dosbox.conf
            cmd = utils::Command{"/opt/retropie/emulators/dosbox/bin/dosbox", { "-conf", config.c_str()}};
        //emulator_ = utils::Process::start(utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch"});

        }
        // TODO append configs and stuff
        // there is no retropie on my dev machine so playing with glxgears instead
#if (defined ARCH_RPI)
        emulator_ = utils::Process::start(cmd);
        //emulator_ = utils::Process::start(utils::Command{"/opt/retropie/emulators/retroarch/bin/retroarch"});
#else
        emulator_ = utils::Process::start(utils::Command{"glxgears"});
#endif
        window().setWidget(this);
    }

protected:
    void tick() override {
        if (emulator_.done())
            window().back();
    }

    void draw() override {
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
        rckid().enableGamepad(true);
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
        rckid().enableGamepad(false);
    }

    // don't do anything here, prevents the back behavior
    void btnB(bool state) { }


    void btnHome(bool state) {
        if (state)
            window().setMenu(& gameMenu_, 0);
    }

    std::string libretroCoreForPath(std::string const & path) {

        char const * ext = GetFileExtension(path.c_str());
        // mgba is used to handle all game boy variants 
        if (strcmp(ext, ".gbc") == 0)
            return "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so";
        if (strcmp(ext, ".gba") == 0)
            return "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so";
        if (strcmp(ext, ".gb") == 0)
            return "/opt/retropie/libretrocores/lr-mgba/mgba_libretro.so";
        // SNES
        if (strcmp(ext, ".sfc") == 0)
            return "/opt/retropie/libretrocores/lr-snes9x2005/snes9x2005_libretro.so";
        // Sega Genesis (MegaDrive)
        if (strcmp(ext, ".md") == 0)
            return "/opt/retropie/libretrocores/lr-genesis-plus-gx/genesis_plus_gx_libretro.so";
        // Nintendo 64
        if (strcmp(ext, ".n64") == 0)
            return "/opt/retropie/libretrocores/lr-mupen64plus/mupen64plus_libretro.so";
        // Dosbox
        if (strcmp(ext, ".EXE") == 0 || strcmp(ext, ".exe"))
            return "/opt/retropie/libretrocores/";
        // TODO change to a decent exception
        UNIMPLEMENTED;
    }

    utils::Process emulator_;

    Menu gameMenu_;
    

}; // RetroArch