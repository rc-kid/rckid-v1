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

#include "raylib_cpp.h"
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

    static Window & create(); 

    Font loadFont(std::string const & filename, int size);

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

    void addFooterItem(FooterItem item) {
        item.textSize_ = MeasureTextEx(helpFont_, item.text_.c_str(), 16, 1.0);
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

    Font const & menuFont() const { return menuFont_; }
    Font const & helpFont() const { return helpFont_; }
    Font const & headerFont() const { return headerFont_; }

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

    void drawFrame(int x, int y, int width, int height, std::string const & title, ::Color color) {

        BeginBlendMode(BLEND_ALPHA);
        drawShapedFrame(x, y, width, height, 10, color);
        drawShapedFrame(x + 2, y + 2, width - 4, height - 4, 10, BLACK);
        static auto t = LoadTexture("assets/images/014-info.png");
        if (t.mipmaps == 0) {
            SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
            GenTextureMipmaps(&t);
        }
        DrawTextureEx(t, Vector2{x + 4.f, y + 4.f}, 0, 0.16, WHITE);
        DrawTextEx(headerFont_, "Information", x + 36, y+4, 20,1, WHITE);




/*
        DrawCircleSector(Vector2{125,40}, 20, 90, 180, 8, DARKGRAY);
        DrawCircleSector(Vector2{105, 195}, 40, 0, 90, 16, DARKGRAY);


        DrawRectangleLines(x, y, width, height, color);
        int theight = MeasureText(helpFont_, title.c_str(), 16, 1.0).x;
        DrawRectangle(x, y - 17, theight + 6, 18, color);
        BeginScissorMode(x + 3, y - 17, theight, 18);
        //DrawTextEx(helpFont_, title.c_str(), x + 3, y-18, 16,1, BLACK);
        DrawTextEx(helpFont_, "Hello all how are things", x + 3, y-16, 16,1, BLACK);
        EndScissorMode();
        DrawRectangleRounded(Rectangle{x - 32.f, y - 32.f, 64.f, 64.f}, 0.5, 8, color);
//        static auto t = LoadTexture("assets/images/014-info.png");
//        DrawTextureEx(t, Vector2{x - 32.f, y - 32.f}, 0, 0.5, WHITE);
*/
    }

    void drawShapedFrame(float x, float y, float w, float h, float r, ::Color color) {
        DrawCircleSector(Vector2{x + r, y + r}, r, 180, 270, 8, color);
        DrawCircleSector(Vector2{x + r, y + h - r}, r, 270, 360, 8, color);
        DrawCircleSector(Vector2{x + w - r, y + r}, r, 90, 180, 8, color);
        DrawCircleSector(Vector2{x + w - 2 * r, y + h - 2 * r}, 2 * r, 0, 90, 8, color);
        DrawRectangle(x, y + r, r, h - 2 *r, color);
        DrawRectangle(x + r, y, w - 3 * r, h, color);
        DrawRectangle(x + w - 2 * r, y, r, r, color);
        DrawRectangle(x + w - 2 * r, y + r, 2 * r, h - 3 * r, color);
    }

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
    void drawFooter();

    void enter(Widget * widget);
    void leave(Widget * widget, bool navPop); 

    // true if the current event should be cancelled
    bool cancelEvent_ = false;

    std::vector<FooterItem> footer_;

    Timepoint lastFrameTime_;
    size_t redrawDelta_; 
    //double lastDrawTime_;
    //float redrawDelta_;
    Font helpFont_;
    Font headerFont_;
    Font menuFont_;
    std::unordered_map<std::string, Font> fonts_;


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

    static inline std::unique_ptr<Window> singleton_;

}; // Window