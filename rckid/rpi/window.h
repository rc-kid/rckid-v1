#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>


#include "raylib_cpp.h"
#include "events.h"
#include "menu.h"
#include "animation.h"
#include "widget.h"
#include "rckid.h"


static constexpr int Window_WIDTH = 320;
static constexpr int Window_HEIGHT = 240;
static constexpr int MENU_FONT_SIZE = 64;
static constexpr int HEADER_HEIGHT = 20;
static constexpr int FOOTER_HEIGHT = 20;

static constexpr char const * HELP_FONT = "IosevkaNF.ttf";
static constexpr char const * MENU_FONT = "OpenDyslexicNF.otf";

class Carousel;
class Keyboard;
class Window;

class FooterItem {
public:
    enum class Control {
        A, B, X, Y, L, R, Left,Right, Up, Down, Select, Start, Home, 
    };

    Control control() const { return control_; }
    std::string const & text() const { return text_; }

    FooterItem(Control control, std::string const & text):
        control_{control}, 
        text_{text} {
    }

    static FooterItem A(std::string const & text) { return FooterItem(Control::A, text); }
    static FooterItem B(std::string const & text) { return FooterItem(Control::B, text); }
    static FooterItem X(std::string const & text) { return FooterItem(Control::X, text); }
    static FooterItem Y(std::string const & text) { return FooterItem(Control::Y, text); }

private:

    friend class Window;

    int draw(Window * window, int x, int y) const ;

    Control control_;
    std::string text_;
    Vector2 textSize_;

}; // FooterItem


/** Basic class for the on-screen Window elements used by RCKid. 
 
    # Header

    The header displays the status of RCKid. It offers a simple and full mode, where the full mode is used when inside the home menu. The simple mode is intended for kids alone and consists of icons only, whereas the menu for adults contains also text information. 

    # Footer

    Footer usually displays only a help with what buttons do what action. 

 */
class Window {
public:

    Window();

    RCKid * rckid() { return rckid_; }

    Font loadFont(std::string const & filename, int size);

    void loop(); 

    /** Shows the error in a modal window. 
     
        TODO
     */
    void error(std::string const & message) {
        TraceLog(LOG_ERROR, message.c_str());
    }

    /** Displays a modal text prompt with the keyboard widget. Calls the provided callback when the input value is confirmed.
     */
    void prompt(std::string const & prompt, std::string value, std::function<void(std::string)> callback);

    void setWidget(Widget * widget);
    void setMenu(Menu * menu, size_t index = 0);
    void setHomeMenu() { setMenu(homeMenu_, 0); }
    void back(); 

    /** Returns current active widget. This is nullptr if we are currently transitioning between widgets. 
    */
    Widget * activeWidget() const { 
        return swap_.running() ? nullptr : widget_;
    }

    /** Returns the time increase since drawing of the last frame begun in milliseconds. Useful for advancing animations and generally keeping pace. 
     */    
    float redrawDelta() const { return redrawDelta_; }

    void resetFooter() {
        footer_.clear();
        if (nav_.size() > 0)
            addFooterItem(FooterItem::B("Back"));
    }

    void addFooterItem(FooterItem item) {
        item.textSize_ = MeasureTextEx(helpFont_, item.text_.c_str(), 16, 1.0);
        footer_.push_back(item);
    }

    void clearFooter() {
        footer_.clear();
    }

    Font const & menuFont() const { return menuFont_; }
    Font const & helpFont() const { return helpFont_; }

    void drawBackground() {
        DrawRectangle(0,0,320,240, BLACK);
        BeginBlendMode(1);
        DrawTexture(background_, backgroundSeam_ - 320, 0, ColorAlpha(WHITE, 0.3));
        DrawTexture(background_, backgroundSeam_, 0, ColorAlpha(WHITE, 0.3));
        EndBlendMode();
    }

    void drawBackground(int seam) {
        setBackgroundSeam(seam);
        drawBackground();
    }

    void setBackgroundSeam(int value) {
        if (value > 320)
            value -= 320;
        else if (value < 0)
            value += 320;
        backgroundSeam_ = value;
    }

    int backgroundSeam() const { return backgroundSeam_; }

private:

    friend class WindowElement;
    friend class Widget;
    friend class RCKid;

    class NavigationItem {
    public:
        enum class Kind {
            Menu, 
            Widget
        }; 

        Kind kind;
        
        Widget * widget() const {
            return kind == Kind::Widget ? widget_ : nullptr;
        }

        Menu * menu() const {
            return kind == Kind::Menu ? menu_ : nullptr;
        }

        size_t menuIndex() const {
            ASSERT(kind == Kind::Menu);
            return menuIndex_;
        }

        NavigationItem(): kind{Kind::Widget}, widget_{nullptr} {}

        NavigationItem(Widget * widget):
            kind{Kind::Widget},
            widget_{widget} {
        }

        NavigationItem(Menu * menu, size_t index):
            kind{Kind::Menu},
            menu_{menu},
            menuIndex_{index} {
        }

    private:

        union {
            Widget * widget_;
            Menu * menu_;
        };

        size_t menuIndex_ = 0;
    }; 

    void draw();

    void btnVolUp(bool state) { 
        if (state) 
            rckid_->setVolume(rckid_->volume() + AUDIO_VOLUME_STEP);  
    }

    void btnVolDown(bool state) {
        if (state) 
            rckid_->setVolume(rckid_->volume() - AUDIO_VOLUME_STEP);  
    }

    void swapWidget();

    void drawHeader();
    void drawFooter();

    // the rckid driver
    RCKid * rckid_; 

    // true if the current event should be cancelled
    bool cancelEvent_ = false;


    std::vector<FooterItem> footer_;

    double lastDrawTime_;
    float redrawDelta_;
    Font helpFont_;
    Font headerFont_;
    Font menuFont_;
    std::unordered_map<std::string, Font> fonts_;
    
    NavigationItem next_;

    /// The currently visible widget
    Widget * widget_ = nullptr;
    /// the carousel used for the menus 
    Carousel * carousel_; 
    /// the keyboard widget for modal string input 
    Keyboard * keyboard_ = nullptr;

    /// The home menu -- TODO should this be in window or outside of it? 
    Menu * homeMenu_;

    std::vector<NavigationItem> nav_;
    bool inHomeMenu_ = false;

    enum class Transition {
        FadeIn, 
        FadeOut, 
        None, 
    }; 

    Animation swap_{250};
    Transition transition_ = Transition::None;

    Texture2D background_;
    int backgroundSeam_ = 160;

    static constexpr int GLYPHS[] = {
        32, 33, 34, 35, 36,37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, // space & various punctuations
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, // 0..9
        58, 59, 60, 61, 62, 63, 64, // more punctuations
        65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, // A-Z
        91, 92, 93, 94, 95, 96, // more punctuations
        97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, // a-z
        123, 124, 125, 126, // more punctiations
        0xf004, 0xf08a, //  
        0xf05a9, 0xf05aa, 0xf16c1, // 󰖩 󰖪 󱛁
        0xf244, 0xf243, 0xf242, 0xf241, 0xf240, 0xf0e7, //      
        0xf02cb, // 󰋋
        0xf1119, // 󱄙
        0xf0e08, 0xf057f, 0xf0580, 0xf057e, // 󰸈 󰕿 󰖀 󰕾
    };

}; // Window