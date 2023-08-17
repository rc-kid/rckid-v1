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
#include <memory>

#include "utils/time.h"

#include "events.h"
#include "animation.h"
#include "widget.h"
#include "rckid.h"
#include "canvas.h"

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

Window & window();

class FooterItem {
public:
    enum class Control {
        A, B, X, Y, L, R, Left,Right, Up, Down, Select, Start, Home, 
        LeftRight, UpDown, DPad,
    };

    Control control() const { return control_; }
    std::string const & text() const { return text_; }

    FooterItem(Control control, std::string const & text):
        control_{control}, 
        text_{text} {
    }

    static FooterItem A(std::string const & text) { return FooterItem{Control::A, text}; }
    static FooterItem B(std::string const & text) { return FooterItem{Control::B, text}; }
    static FooterItem X(std::string const & text) { return FooterItem{Control::X, text}; }
    static FooterItem Y(std::string const & text) { return FooterItem{Control::Y, text}; }
    static FooterItem Select(std::string const & text) { return FooterItem{Control::Select, text}; }
    static FooterItem UpDown(std::string const & text) { return FooterItem{Control::UpDown, text}; }
    static FooterItem LeftRight(std::string const & text) { return FooterItem{Control::LeftRight, text}; }

private:

    friend class Window;

    int draw(Canvas & canvas, int x, int y) const;

    Control control_;
    std::string text_;
    int textWidth_;

}; // FooterItem


/** Basic class for the on-screen Window elements used by RCKid. 
 
    # Header

    The header displays the status of RCKid. It offers a simple and full mode, where the full mode is used when inside the home menu. The simple mode is intended for kids alone and consists of icons only, whereas the menu for adults contains also text information. 

    # Footer

    Footer usually displays only a help with what buttons do what action. 

 */
class Window {
public:

    static Window & create(); 

    Canvas & canvas() { return *canvas_; }

    //Font loadFont(std::string const & filename, int size);

    void loop(); 

    void showWidget(Widget * widget);
    void showHomeMenu();
    void back(size_t numWidgets = 1);

    /** Number of widgets in the navigation stack. 
     
        Number greater than 0 will display the back key icon in the footer automatically. 
     */
    size_t navSize() const { return nav_.size(); }

    /** Displays the widget modally, i.e. over the currently active widget. 
     */
    void showModal(Widget * widget);

    /** Returns current active widget.  
     */
    Widget * activeWidget() const {
        if (nav_.empty() || aSwap_.running())
            return nullptr;
        return modal_ != nullptr ? modal_ : nav_.back();
    }

    /** Returns the time increase since drawing of the last frame begun in milliseconds. Useful for advancing animations and generally keeping pace. 
     */    
    float redrawDelta() const { return redrawDelta_; }

    void resetFooter(bool forceBack = false) {
        footer_.clear();
        if (nav_.size() > 1 || forceBack)
            addFooterItem(FooterItem::B("Back"));
        redrawFooter_ = true;
    }

    void clearFooter() {
        footer_.clear();
        redrawFooter_ = true;
    }

    void addFooterItem(FooterItem item) {
        item.textWidth_ = canvas_->textWidth(item.text_, canvas_->helpFont());
        footer_.push_back(item);
        redrawFooter_ = true;
    }
    
    void showFooter() {
        if (!footerVisible_) {
            tFooter_ = Transition::FadeIn;
            aFooter_.start();
            footerVisible_ = true;
        }
    }

    void hideFooter() {
        if (footerVisible_) {
            tFooter_ = Transition::FadeOut;
            aFooter_.start();
            footerVisible_ = false;
        }
    }

    void showHeader() {
        if (!headerVisible_) { 
            tHeader_ = Transition::FadeIn;
            aHeader_.start();
            redrawHeader_ = true;
            headerVisible_ = true;
        }
    }

    void hideHeader() {
        if (headerVisible_) {
            tHeader_ = Transition::FadeOut;
            aHeader_.start();
            headerVisible_ = false;
        }
    }

    void enableBackground(bool value) { backgroundOpacity_ = value ? 255 : 0; }

    void enableBackgroundDark(uint8_t opacity) { backgroundOpacity_ = opacity; }
 
    void setBackgroundSeam(int value) {
        if (value > 320)
            value -= 320;
        else if (value < 0)
            value += 320;
        if (backgroundSeam_ != value) {
            backgroundSeam_ = value;
            redrawBackground_ = true;
        }
    }

    int backgroundSeam() const { return backgroundSeam_; }

private:

    friend class WindowElement;
    friend class Widget;
    friend class ModalWidget;
    friend class RCKid;

    friend Window & window() { return * Window::singleton_; }

    enum class Transition {
        FadeIn, 
        FadeOut, 
        None, 
    }; 

    /** Private constructor for the singleton. 
     */
    Window();

    void draw();

    void drawHeader();
    void drawBatteryGauge(int & x, uint16_t vbatt);
    void drawFooter();

    void enter(Widget * widget);
    void leave(Widget * widget, bool navPop); 

    // true if the current event should be cancelled
    bool cancelEvent_ = false;

    std::vector<FooterItem> footer_;

    std::unique_ptr<Canvas> canvas_;

    Timepoint lastFrameTime_;
    size_t redrawDelta_; 


    Animation aSwap_{WIDGET_FADE_TIME};
    Transition tSwap_ = Transition::None;

    /** Widget navigation stack. 
     */    
    std::vector<Widget*> nav_;

    Animation aModal_{WIDGET_FADE_TIME};
    Transition tMOdal_ = Transition::None; 

    /** Modal widget.
     */
    Widget * modal_ = nullptr;

    /// The home menu -- TODO should this be in window or outside of it? 
    Carousel * homeMenu_;


    Texture2D background_;
    int backgroundSeam_ = 160;
    uint8_t backgroundOpacity_ = 255;

    /** Because of the abysmally slow 2D drawing performance on RPi we buffer all the UI elements in rendering textures are only redraw them when necessary. 
    */
    RenderTexture2D backgroundCanvas_;
    RenderTexture2D widgetCanvas_;
    RenderTexture2D modalCanvas_;
    RenderTexture2D headerCanvas_;
    RenderTexture2D footerCanvas_;

    bool redrawBackground_ = true;
    bool redrawHeader_ = true;
    bool redrawFooter_ = true;

    /** Transition we use to determine the header visibility. 
     */
    Animation aHeader_{WIDGET_FADE_TIME};
    Transition tHeader_ = Transition::None;
    bool headerVisible_ = true;

    Animation aFooter_{WIDGET_FADE_TIME};
    Transition tFooter_ = Transition::None;
    bool footerVisible_ = true;


    static inline std::unique_ptr<Window> singleton_;

}; // Window

