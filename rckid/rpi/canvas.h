#pragma once

#include <memory>


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

inline constexpr Vector2 V2(float x, float y) { return Vector2{x, y}; }
inline constexpr Vector2 V2(int x, int y) { return Vector2{static_cast<float>(x), static_cast<float>(y)}; }


#endif

/** Wrapper around the GUI interface. 
 
    This is mostly to make raylib's C style resource allocation and drawing primitives simpler and easier.
*/
class Canvas {
public:

    /** Wrapper over 2D textures. 
     */
    class Texture {
    public:
        int width() const { return t_->t.width; }
        int height() const { return t_->t.height; };
        std::string const & filename() const { return t_->filename; }

        Texture() = default;

        Texture(std::string const & filename) {
            auto i = cached_.find(filename);
            if (i == cached_.end())
                i = cached_.insert(
                    std::make_pair(
                        filename, 
                        std::make_shared<TextureInfo>(
                            LoadTexture(filename.c_str()), 
                            filename
                        )
                    )
                ).first;
            t_ = i->second;
        }

        ~Texture() {
            if (t_.use_count() == 2) {
                cached_.erase(t_->filename);
                TraceLog(LOG_INFO, STR("Unloading texture " << t_->filename << " (id: " << t_->t.id << ")"));
                UnloadTexture(t_->t);
            }
        }

        bool valid() const { return t_ != nullptr; }

    private:

        friend class Canvas;

        struct TextureInfo {
            Texture2D t;
            std::string filename;

            TextureInfo(Texture2D t, std::string const & filename): t{t}, filename{filename} {}

        }; 

        Texture(std::shared_ptr<TextureInfo> t): t_{t} {}

        Texture2D t2d() const { return t_->t; }

        std::shared_ptr<TextureInfo> t_;

        static inline std::unordered_map<std::string, std::shared_ptr<TextureInfo>> cached_; 
    }; 

    Canvas(int width, int height):
        width_{width}, 
        height_{height} {
    }

    ~Canvas() {
    }
    
    void drawTexture(int x, int y, Texture const & t) {
        DrawTexture(t.t2d(), x, y, WHITE);
    }

    void drawTexture(int x, int y, Texture const & t, int alpha) {
        DrawTexture(t.t2d(), x, y, ColorAlpha(WHITE, alpha / 255.f));
    }

    void drawTexture(int x, int y, Texture const & t, Color const & tint) {
        DrawTexture(t.t2d(), x, y, tint);
    }

    void drawTextureScaled(int x, int y, Texture const & t, float scale, Color const & tint) {
        DrawTextureEx(t.t2d(), V2(x, y), 0, scale, tint);
    }



    // YE OLDE STUFF THAT MIGHT BE USEFUL ONE DAY

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
        //DrawTextEx(window().headerFont(), "Information", x + 36, y+4, 20,1, WHITE);




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

    void drawProgressBar(int x, int y, int width, int height, float progress, ::Color bg, ::Color fg) {
        float radius = height / 2.0;
        DrawCircleSector(Vector2{x + radius, y + radius}, radius, 180, 360, 16, bg);
        DrawRectangle(x + radius, y, width - height, height, bg);
        DrawCircleSector(Vector2{x + width - radius, y + radius}, radius, 0, 180, 16, bg);
        BeginScissorMode(x, y, width * progress, height);
        DrawCircleSector(Vector2{x + radius, y + radius}, radius, 180, 360, 16, fg);
        DrawRectangle(x + radius, y, width - height, height, fg);
        DrawCircleSector(Vector2{x + width - radius, y + radius}, radius, 0, 180, 16, fg);
        EndScissorMode();        
    }        



private:

    int width_;
    int height_;


#ifdef FOO    

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


    Font & loadFont(std::string const & filename, int size) {
        FontID id{filename, size};
        auto i = fonts_.find(id);
        if (i == fonts_.end()) {
            ::Font f = LoadFontEx(filename.c_str(), size, const_cast<int*>(GLYPHS), sizeof(GLYPHS) / sizeof(int));
            i = fonts_.insert(std::make_pair(id, new Font{f})).first;
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

#endif  

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