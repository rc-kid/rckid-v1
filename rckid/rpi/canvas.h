#pragma once
#include "raylib.h"

#include "utils/utils.h"

#ifdef FOO

/** Allows log function to take std::string as well. 
 */
inline void TraceLog(int loglevel, std::string const & str) {
    TraceLog(loglevel, str.c_str());
}

/** Creates an opaque color from its constituent parts. 
 */
inline constexpr Color RGB(uint8_t r, uint8_t g, uint8_t b) {
    return ::Color{r, g, b, 255};
}

/** Creates an RGB color with given opacity (0 == fully transparent, 255 = fully opaque). 
 */
inline constexpr Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ::Color{r, g, b, a};
}

#endif

/** Wrapper around the GUI interface. 
 
    This is mostly to make raylib's C style resource allocation and drawing primitives simpler and easier.
*/
class Canvas {
public:

    /** Wrapper class for fonts. 
     */
    class Font {
    public:
        ~Font() {
            UnloadFont(font_);
        }

        int height() const { return font_.baseSize; }

    private:
        friend class Canvas;
        Font(::Font f): font_{f} {}

        ::Font font_;
    }; 

    /** Wrapper class for textures. 
     */
    class Texture {
    public:
        ~Texture() {
            UnloadTexture(texture_);
        }

        int width() const { return texture_.width; }
        int height() const { return texture_.height; }
    private:
        friend class Canvas;
        Texture(Texture2D t): texture_{t} {}

        Texture2D texture_;
    }; 

    Canvas(int width, int height, std::string const & windowTitle) {
        InitWindow(width, height, windowTitle.c_str());
        SetTargetFPS(60);
        if (! IsWindowReady())
            TraceLog(LOG_ERROR, "Unable to initialize window");
    }

    ~Canvas() {
        CloseWindow();
        for (auto & f : fonts_)
            delete f.second;
        for (auto & t : textures_)
            delete t.second;
    }

    Font & loadFont(std::string const & filename, int size) {
        FontID id{filename, size};
        auto i = fonts_.find(id);
        if (i == fonts_.end()) {
            ::Font f = LoadFontEx(filename.c_str(), size, const_cast<int*>(GLYPHS), sizeof(GLYPHS) / sizeof(int));
            i = fonts_.insert(std::make_pair(id, new Font{f})).first;
        }
        return *(i->second);
    }

    Texture & loadTexture(std::string const & filename) {
        auto i = textures_.find(filename);
        if (i == textures_.end()) {
            Texture2D t = LoadTexture(filename.c_str());
            i = textures_.insert(std::make_pair(filename, new Texture{t})).first;
        }
        return *(i->second);
    }

    void setFont(Font & font) { font_ = & font; }

    Font const & font() const { return *font_; }

    void setFg(Color color) { fg_ = color; }

    Color const & fg() const { return fg_; }

    void setBg(Color color) { bg_ = color; }

    Color const & bg() const { return bg_; }

    int textHeight(std::string const & text) {
        return static_cast<int>(MeasureTextEx(font_->font_, text.c_str(), font_->height(), 1.0).x);
    }

    void textOut(int x, int y, std::string const & text) {
        DrawTextEx(font_->font_, text.c_str(), Vector2{x + 0.f, y + 0.f}, font_->height(), 1.0, fg_);
    }

    void drawTexture(int x, int y, Texture2D const & t) {
        DrawTexture(t, x, y, WHITE);
    }

    void drawTexture(int x, int y, Texture2D const & t, int alpha) {
        DrawTexture(t, x, y, ColorAlpha(WHITE, alpha / 255.f));
    }

    void drawTexture(int x, int y, Texture2D const & t, Color const & tint) {
        DrawTexture(t, x, y, tint);
    }

private:

    struct FontID {
        std::string file;
        int size;

        bool operator == (FontID const & other) const { return file == other.file && size == other.size; }

        size_t hash() const {
            size_t h1 = std::hash<std::string>{}(file);
            size_t h2 = std::hash<uint16_t>{}(size);
            return h1 ^ (h2 << 1);
        }

    }; 

    std::unordered_map<FontID, Font *, Hasher<FontID>> fonts_;
    std::unordered_map<std::string, Texture *> textures_;

    Font * font_ = nullptr;
    Color fg_;
    Color bg_;

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

}; // Canvas