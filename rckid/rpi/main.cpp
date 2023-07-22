#ifdef ARCH_RPI
#include "bcm_host.h"

struct DispmanXBackgroundLayer {
    int32_t layer;
    DISPMANX_RESOURCE_HANDLE_T resource;
    DISPMANX_ELEMENT_HANDLE_T element;
};

#endif


#include "raylib_cpp.h"

#include "platform/platform.h"

#include "rckid.h"
#include "window.h"
#include "menu.h"
#include "pixel_editor.h"
#include "debug_view.h"
#include "keyboard.h"
#include "game_player.h"
#include "music_player.h"
#include "video_player.h"
#include "torchlight.h"
#include "recorder.h"
#include "walkie_talkie.h"
#include "remote.h"
#include "nrf_sniffer.h"

void printLogLevel(int logLevel, std::ostream & s) {
    switch (logLevel) {
        case LOG_TRACE:
            s << "TRACE";
            break;
        case LOG_DEBUG:
            s << "DEBUG";
            break;
        case LOG_INFO:
            s << "INFO";
            break;
        case LOG_WARNING:
            s << "WARNING";
            break;
        case LOG_ERROR:
            s << "ERROR";
            break;
        case LOG_FATAL:
            s << "FATAL";
            break;
        default:
            s << "??? (" << logLevel << ")";
            break;
    }
}

void logCallback(int logLevel, const char * text, va_list args) {
    static std::ofstream logFile{"/rckid/log.txt"};
    static size_t bufferSize = 1024;
    static char * buffer = new char[bufferSize];
    int len = vsnprintf(buffer, bufferSize, text, args);
    if (len >= bufferSize) {
        bufferSize = len + 1;
        delete [] buffer;
        buffer = new char[bufferSize];
        len = vsnprintf(buffer, bufferSize, text, args);
    }
    if (len >= 0) {
        buffer[len] = '\0';
#ifndef NDEBUG
        printLogLevel(logLevel, std::cout);
        std::cout << ": " << buffer << std::endl;
#endif
        printLogLevel(logLevel, logFile);
        logFile << ": " << buffer << std::endl;
        logFile.flush();
    }
}

/** Main RCKid app. 
 
 */
int main(int argc, char * argv[]) {
    system("cp /rckid/log.txt /rckid/log.old.txt");
    SetTraceLogLevel(LOG_ALL);
    SetTraceLogCallback(logCallback);
#ifdef ARCH_RPI
    // make sure that we have the rights to add uinput device    
    system("sudo chown pi /dev/uinput");
    // add layer 0 black rectangle to block framebuffer, we don't care about errors,
    // TODO Maybe add some nice picture
    bcm_host_init();
    DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);
    DISPMANX_RESOURCE_HANDLE_T resource;
    DISPMANX_ELEMENT_HANDLE_T element;
    // init bg layer
    {
        VC_IMAGE_TYPE_T type = VC_IMAGE_RGBA16;
        uint32_t vc_image_ptr;
        resource = vc_dispmanx_resource_create(type, 1, 1, &vc_image_ptr);
        VC_RECT_T dst_rect;
        vc_dispmanx_rect_set(&dst_rect, 0, 0, 1, 1);
        uint16_t color = 0x000f; // black
        vc_dispmanx_resource_write_data(resource, type, sizeof(color), &color, &dst_rect);
    }
    // draw the bg layer 
    {
        DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);    
        VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 255, 0 };
        VC_RECT_T src_rect;
        vc_dispmanx_rect_set(&src_rect, 0, 0, 1, 1);
        VC_RECT_T dst_rect;
        vc_dispmanx_rect_set(&dst_rect, 0, 0, 0, 0);
        element = vc_dispmanx_element_add(
            update,
            display,
            0, // layer
            &dst_rect,
            resource,
            &src_rect,
            DISPMANX_PROTECTION_NONE,
            &alpha,
            nullptr,
            DISPMANX_NO_ROTATE);
        vc_dispmanx_update_submit_sync(update);
    }
#endif
    try {
        Window::create();
        RCKid::create();
        GamePlayer gamePlayer{};
        VideoPlayer videoPlayer{};
        MusicPlayer musicPlayer{};
        rckid().setVolume(AUDIO_DEFAULT_VOLUME);
        Menu menu{{
            new LazySubmenu{
                "Games",
                "assets/images/001-game-controller.png", 
                [&]() {
                    return new JSONMenu{
                        "/rckid/games/folder.json",
                        [&](json::Value & item) {
                            gamePlayer.play(item);
                        }
                    };
                }
            },
            new LazySubmenu{
                "Video", 
                "assets/images/005-film-slate.png",
                [&]() {
                    return new JSONMenu{
                        "/rckid/videos/folder.json",
                        [&](json::Value & item) {
                            videoPlayer.play(item);
                        }
                    };
                }
            },
            new LazySubmenu{
                "Music", 
                "assets/images/003-music.png",
                [&]() {
                    return new JSONMenu{
                        "/rckid/music/folder.json",
                        [&](json::Value & item) {
                            musicPlayer.play(item);
                        }
                    };
                }
            },
            //new JSON{"Music", "assets/images/003-music.png", "/rckid/music/folder.json", &window},
            //new Menu::Item{"Remote", "assets/images/002-rc-car.png"},
            new WidgetItem{"Remote", "assets/images/002-rc-car.png", new Remote{}},
            new WidgetItem{"Walkie-Talkie", "assets/images/007-baby-monitor.png", new WalkieTalkie{}},
            new Submenu{"Apps", "assets/images/022-presents.png", {
                new WidgetItem{"Torchlight", "assets/images/004-flashlight.png", new Torchlight{}},
                new WidgetItem{"Paint", "assets/images/053-paint-palette.png", new PixelEditor{}},
                new Menu::Item{"Baby Monitor", "assets/images/006-baby-crib.png"},
                new WidgetItem{"Recording", "assets/images/026-magic-wand.png", new Recorder{}},
                new WidgetItem{"NRF Sniffer", "assets/images/084-spy.png", new NRFSniffer{}},
            }},
        }};
        //Keyboard kb{&window};
        //window.setWidget(&kb);
        //DebugView db{&window};
        //window.setWidget(&db);
        window().setMenu(& menu, 0);
        window().loop();
    } catch (std::exception const & e) {
        TraceLog(LOG_FATAL, e.what());
    } catch (...) {
        TraceLog(LOG_FATAL, "Unknown exception thrown");
    }
#ifdef ARCH_RPI
    // destroy bg layer
    {
        DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
        vc_dispmanx_element_remove(update, element);
        vc_dispmanx_update_submit_sync(update);
        vc_dispmanx_resource_delete(resource);
    }
#endif
}