#pragma once

#include <memory>

#include "utils/utils.h"
#include "raylib_wrapper.h"
#include "animation.h"


/** Wrapper around the GUI interface. 
 
    This is mostly to make raylib's C style resource allocation and drawing primitives simpler and easier.
*/
class Canvas {
public:

    /** Wrapper around a font. 
     */
    class Font {
    public:
        int size() const { return f_->font.baseSize; }
        std::string const & filename() const { return f_->filename; };

        bool valid() const { return f_ != nullptr; }

        Font() = default;

        Font(std::string const & filename, int size) {
            std::string id{STR(filename << "."  << size)};
            auto i = cached_.find(id);
            if (i == cached_.end()) {
                i = cached_.insert(
                    std::make_pair(
                        id, 
                        std::make_shared<FontInfo>(
                            LoadFontEx(
                                filename.c_str(), 
                                size, 
                                const_cast<int*>(GLYPHS), 
                                sizeof(GLYPHS) / sizeof(int)
                            ), 
                            filename
                        )
                    )
                ).first;
            }
            f_ = i->second;
        }

        ~Font() {
            if (f_.use_count() == 2) {
                std::string id{STR(filename() << "."  << size())};
                cached_.erase(id);
                TraceLog(LOG_INFO, STR("Unloading font " << f_->filename << " (size: " << f_->font.baseSize << ")"));
                UnloadFont(f_->font);
            }
        }

    protected:

        friend class Canvas;
        
        struct FontInfo {
            ::Font font;
            std::string filename;

            FontInfo(::Font f, std::string const & filename): font{f}, filename{filename} {}
        }; 

        std::shared_ptr<FontInfo> f_;

        static inline std::unordered_map<std::string, std::shared_ptr<FontInfo>> cached_; 

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
            0xf04d, 0xf04c, 0xf04b, 0xf01e, //     
            0xf0e7a, 0xf0e74, 0xf047 // 󰹺󰹴 
        };

    }; 

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
        aScrolledText_.startContinuous();
    }

    ~Canvas() {
    }

    int width() const { return width_; }
    int height() const { return height_; }

    Font const & defaultFont() const { return defaultFont_; }
    Font const & titleFont() const { return titleFont_; }
    Font const & helpFont() const { return helpFont_; }

    Color accentColor() const { return accentColor_; }

    void resetDefaults() {
        font_ = defaultFont_;
        fg_ = WHITE;
        bg_ = BLACK;
    }

    /** \name State setters and getters. 
     */
    //@{
    void setFg(Color color) { fg_ = color; }

    Color const & fg() const { return fg_; }

    void setBg(Color color) { bg_ = color; }

    Color const & bg() const { return bg_; }

    void setFont(Font const & font) { font_ = font; }

    void setDefaultFont() { font_ = defaultFont_; }

    Font const & font() const { return font_; }
    //@}


    /** \name Texture drawing support. 
     */
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
    //@}

    /** \name Font drawing support. 
     */
    //@{

    int textWidth(std::string const & text) {
        return static_cast<int>(MeasureTextEx(font_.f_->font, text.c_str(), font_.size(), 1.0).x);
    }

    int textWidth(std::string const & text, Font const & font) {
        return static_cast<int>(MeasureTextEx(font.f_->font, text.c_str(), font.size(), 1.0).x);
    }

    void drawText(int x, int y, std::string const & text) {
        DrawTextEx(font_.f_->font, text.c_str(), V2(x, y), font_.size(), 1.0, fg_);
    }

    void drawText(int x, int y, std::string const & text, Color c) {
        DrawTextEx(font_.f_->font, text.c_str(), V2(x, y), font_.size(), 1.0, c);
    }

    void drawText(int x, int y, std::string const & text, Color c, Font const & font) {
        DrawTextEx(font.f_->font, text.c_str(), V2(x, y), font.size(), 1.0, c);
    }

    bool drawScrolledText(int x, int y, int displayWidth, std::string const & text, int textWidth, Color c) {
        return drawScrolledText(x, y, displayWidth, text, textWidth, c, defaultFont_);
    }   


    bool drawScrolledText(int x, int y, int displayWidth, std::string const & text, int textWidth, Color c, Font const & f) {
        if (displayWidth >= textWidth) {
            drawText(x + (displayWidth - textWidth) / 2, y, text, c, f);
            return false;
        } else {
            int dist = (textWidth - displayWidth) / 2;
            int offset = aScrolledText_.interpolateContinuous(0, dist) * scrolledTextDir_;
            BeginScissorMode(x, y, displayWidth, f.size());
            drawText(x + (displayWidth - textWidth) / 2 +  offset, y, text, c, f);
            EndScissorMode();
            return true;
        }
    }

    //@}

    void fillFrame(int x, int y, int w, int h, ::Color color) {
        int r = 10;
        DrawCircleSector(V2(x + r, y + r), r, 180, 270, 8, color);
        DrawCircleSector(V2(x + r, y + h - r), r, 270, 360, 8, color);
        DrawCircleSector(V2(x + w - r, y + r), r, 90, 180, 8, color);
        DrawCircleSector(V2(x + w - 2 * r, y + h - 2 * r), 2 * r, 0, 90, 8, color);
        DrawRectangle(x, y + r, r, h - 2 *r, color);
        DrawRectangle(x + r, y, w - 3 * r, h, color);
        DrawRectangle(x + w - 2 * r, y, r, r, color);
        DrawRectangle(x + w - 2 * r, y + r, 2 * r, h - 3 * r, color);
    }

    void drawFrame(int x, int y, int width, int height, ::Color color, int thickness = 2) {
        fillFrame(x, y, width, height, color);
        fillFrame(x + thickness, y + thickness, width - 2 * thickness, height -  2 * thickness, BLACK);
    }


    // YE OLDE STUFF THAT MIGHT BE USEFUL ONE DAY

/*
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
    */   



private:

    friend class Window;

    void update() {
        if (aScrolledText_.update())
            scrolledTextDir_ *= -1;

    }

    int width_;
    int height_;

    Font font_;
    Color fg_;
    Color bg_;

    Font defaultFont_{"assets/fonts/IosevkaNF.ttf", 20};
    Font titleFont_{"assets/fonts/OpenDyslexicNF.otf", 64};
    Font helpFont_{"assets/fonts/IosevkaNF.ttf", 16};

    Animation aScrolledText_{5000};
    int scrolledTextDir_ = 1;





    Color accentColor_{PINK};
}; // Canvas