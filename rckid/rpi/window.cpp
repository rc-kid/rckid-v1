
#include "platform/platform.h"

#include "carousel.h"
#include "keyboard.h"
#include "debug_view.h"
#include "gauge.h"

#include "window.h"


int FooterItem::draw(Window * window, int x, int y) const {
    switch (control_) {
        case Control::A:
            DrawCircle(x + 10, y + 10, 6, RED);
            x += 20;
            break;
        case Control::B:
            DrawCircle(x + 10, y + 10, 6, YELLOW);
            x += 20;
            break;
        case Control::X:
            DrawCircle(x + 10, y + 10, 6, BLUE);
            x += 20;
            break;
        case Control::Y:
            DrawCircle(x + 10, y + 10, 6, GREEN);
            x += 20;
            break;
    }
    DrawTextEx(window->helpFont(), text_.c_str(), x, y + (20 - textSize_.y) / 2 , 16, 1.0, WHITE);
    x += textSize_.x + 5;
    return x;
}

Window::Window() {
    InitWindow(320, 240, "RCKid");
    SetTargetFPS(60);
    if (! IsWindowReady())
        TraceLog(LOG_ERROR, "Unable to initialize window");
    helpFont_ = loadFont(HELP_FONT, 16);
    headerFont_ = loadFont(HELP_FONT, 20);
    menuFont_ = loadFont(MENU_FONT, MENU_FONT_SIZE);
    background_ = LoadTexture("assets/backgrounds/unicorns-black.png");
    backgroundCanvas_ = LoadRenderTexture(640, 240);
    widgetCanvas_ = LoadRenderTexture(320, 240);
    headerCanvas_ = LoadRenderTexture(320, 240);
    footerCanvas_ = LoadRenderTexture(320, 240);
    BeginTextureMode(backgroundCanvas_);
    ClearBackground(ColorAlpha(BLACK, 0.0));
    DrawRectangle(0,0,640,240, BLACK);
    DrawTexture(background_, 0, 0, ColorAlpha(WHITE, BACKGROUND_OPACITY));
    DrawTexture(background_, 320, 0, ColorAlpha(WHITE, BACKGROUND_OPACITY));
    EndTextureMode();

    InitAudioDevice();

    rckid_ = new RCKid{this};
    carousel_ = new Carousel{this};
    homeMenu_ = new Menu{{
        new ActionItem{"Exit", "assets/images/011-power-off.png",[](){
            ::exit(0);
        }},
        new ActionItem{"Power Off", "assets/images/011-power-off.png",[&](){
            rckid_->powerOff();
        }},
        new Menu::Item{"Airplane Mode", "assets/images/012-airplane-mode.png"},
        new WidgetItem{"Brightness", "assets/images/009-brightness.png", new Gauge{this, "assets/images/009-brightness.png", 0, 255, 16, 
            [this](int value) { 
                rckid_->setBrightness(value); 
            },
            [this](Gauge * g) {
                g->setValue(rckid_->brightness());
            }
        }},
        new WidgetItem{"Volume", "assets/images/010-high-volume.png", new Gauge{this, "assets/images/010-high-volume.png", 0, 100, 10,
            [this](int value){
                rckid_->setVolume(value);
            },
            [this](Gauge * g) {
                g->setValue(rckid_->volume());
            }
        }},
        new Menu::Item{"WiFi", "assets/images/016-wifi.png"},
        new WidgetItem{"Debug", "assets/images/021-poo.png", new DebugView{this}},
    }};
    lastFrameTime_ = now();    
}

Font Window::loadFont(std::string const & filename, int size) {
    auto fname = STR(filename << "--" << size);
    auto i = fonts_.find(fname);
    if (i == fonts_.end()) {
        Font f = LoadFontEx(STR("assets/fonts/" << filename).c_str(), size, const_cast<int*>(GLYPHS), sizeof(GLYPHS) / sizeof(int));
        i = fonts_.insert(std::make_pair(fname, f)).first;
    }
    return i->second;
}

void Window::prompt(std::string const & prompt, std::string value, std::function<void(std::string)> callback) {
    if (keyboard_ == nullptr)
        keyboard_ = new Keyboard{this};
    keyboard_->prompt = prompt;
    keyboard_->value = value;
    keyboard_->onDone = callback;
    setWidget(keyboard_);
}

void Window::setWidget(Widget * widget) {
    next_ = NavigationItem{widget};
    transition_ = Transition::FadeOut;
    if (widget_ == nullptr) {
        swapWidget();
    } else {
        if (widget_ == carousel_)
            nav_.push_back(NavigationItem(carousel_->items(), carousel_->index()));
        else
            nav_.push_back(NavigationItem(widget_));
        transition_ = Transition::FadeOut;
        aswap_.start();
        // if we are swapping for a fullscreen widget, hide header in sync with footer
        if (widget->fullscreen())
            hideHeader();
    }
}

void Window::setMenu(Menu * menu, size_t index) {
    if (menu == homeMenu_) {
        if (inHomeMenu_)
            return;
    }
    next_ = NavigationItem{menu, index};
    if (widget_ == nullptr) {
        swapWidget();
    } else {
        if (widget_ == carousel_)
            nav_.push_back(NavigationItem(carousel_->items(), carousel_->index()));
        else 
            nav_.push_back(NavigationItem(widget_));
        transition_ = Transition::FadeOut;
        aswap_.start();
    }
}

void Window::back(size_t numWidgets) {
    if (nav_.empty() || numWidgets == 0)
        return;
    next_ = nav_.back();
    nav_.pop_back();
    while (numWidgets > 1 && ! nav_.empty()) {
        if (next_.kind == NavigationItem::Kind::Widget) {
            ASSERT(next_.widget()->onNavStack_ == true);
            next_.widget()->onNavigationPop();
            next_.widget()->onNavStack_ = false;
        } else if (next_.menu() == homeMenu_) {
            inHomeMenu_ = false;
        }
        next_ = nav_.back();
        nav_.pop_back();
        --numWidgets;
    }
    if (next_.kind == NavigationItem::Kind::Widget && next_.widget()->fullscreen())
        hideHeader();
    if (widget_ == nullptr) {
        swapWidget();
    } else {
        transition_ = Transition::FadeOut;
        aswap_.start();
    }
}

void Window::swapWidget() {
    if (widget_!= nullptr) {
        widget_->onBlur();
        if (widget_ == carousel_) {
            // if we are leaving home menu indicate
            if (carousel_->items() == homeMenu_ && (nav_.empty() || nav_.back().menu() != homeMenu_))
                inHomeMenu_ = false;
            carousel_->items()->onBlur();
        } else {
            if (nav_.empty() || nav_.back().widget() != widget_) {
                ASSERT(widget_->onNavStack_ == true);
                widget_->onNavigationPop();
                widget_->onNavStack_ = false;
            }
        }
    }
    if (next_.kind == NavigationItem::Kind::Menu) {
        widget_ = carousel_;
        carousel_->setItems(next_.menu(), next_.menuIndex());
        if (next_.menu() == homeMenu_)
            inHomeMenu_ = true;
    } else {
        widget_ = next_.widget();
        if (! widget_->onNavStack_) {
            widget_->onNavigationPush();
            widget_->onNavStack_ = true;
        }
    }
    resetFooter();
    widget_->onFocus();
    if (widget_ == carousel_)
        carousel_->items()->onFocus();
    transition_ = Transition::FadeIn;
    if (!widget_->fullscreen())
        showHeader();
    aswap_.start();
    // always redraw header when swapping a widget, just to be sure
    redrawHeader_ = true;
    // and require redraw
    widget_->redraw_ = true;
}

void Window::loop() {
    Event e;
    while (true) {
#if (defined ARCH_MOCK)
        if (WindowShouldClose())
            break;
#endif
        rckid_->loop();
        draw();
    }
}

struct RenderingStats {
    unsigned widget;
    unsigned background;
    unsigned header;
    unsigned footer;
    unsigned render;
    unsigned stats;
    unsigned total;
    unsigned delta;
};

#if (defined RENDERING_STATS)
RenderingStats frames_[320];
size_t fti_ = 0;
#endif

void Window::draw() {
    Timepoint t = now();
    redrawDelta_ = asMillis(t - lastFrameTime_);
    lastFrameTime_ = t;
    aswap_.update(this);
    aheader_.update(this);

    bool redraw = false;


    // Start by rendering the background. 
    //
    // TODO this can be simplified by prerendering the background into twice large texture and then only copy the parts of it as needed according to the seam elliminating the need for the 2 texture draws when background changes
#if (defined RENDERING_STATS)
    frames_[fti_].delta = redrawDelta_;
    Timepoint tt = now();
#endif
    if (redrawBackground_) {
        redraw = true;
        redrawBackground_ = false;
    }
#if (defined RENDERING_STATS)
    //frames_[fti_].background = static_cast<unsigned>((GetTime() - tt) * 1000);
    frames_[fti_].background = asMillis(now() - tt);
#endif

    // Render the widget. 
    //
#if (defined RENDERING_STATS)
    tt = now();
#endif
    if (widget_ != nullptr) {
        if (widget_->redraw_) {
            redraw = true;
            BeginTextureMode(widgetCanvas_);
            ClearBackground(ColorAlpha(BLACK, 0.0));
            BeginBlendMode(BLEND_ADD_COLORS);
            widget_->draw();
            EndBlendMode();
            EndTextureMode();
        } else {
            widget_->idle();
        }
    }
#if (defined RENDERING_STATS)
    frames_[fti_].widget = asMillis(now() - tt);
#endif

    // Render the header
    //
#if (defined RENDERING_STATS)
    tt = now();
#endif
    if (rckid_->statusChanged()) 
        redrawHeader_ = true;
    if (redrawHeader_) {
        redraw = true;
        redrawHeader_ = false;
        BeginTextureMode(headerCanvas_);
        ClearBackground(ColorAlpha(BLACK, 0.0));
        drawHeader();
        EndTextureMode();    
    }
#if (defined RENDERING_STATS)
    frames_[fti_].footer = asMillis(now() - tt);
#endif

    // Render the footer
#if (defined RENDERING_STATS)
    tt = now();
#endif
    if (redrawFooter_) {
        redraw = true;
        redrawFooter_ = false;
        BeginTextureMode(footerCanvas_);
        ClearBackground(ColorAlpha(BLACK, 0.0));
        drawFooter();
        EndTextureMode();
    }
#if (defined RENDERING_STATS)
    frames_[fti_].footer = asMillis(now() - tt);
#endif

    // redraw if transition of either the widget or the header is in progress 
    redraw = redraw || transition_ == Transition::FadeIn || transition_ == Transition::FadeOut || header_ == Transition::FadeIn || header_ == Transition::FadeOut;

    // finally, piece together the actual frame
    if (redraw) {
#if (defined RENDERING_STATS)
        tt = now();
#endif
        ::Color widgetAlpha = WHITE;
        float footerOffset = 0;
        switch (transition_) {
            case Transition::FadeOut:
                footerOffset = aswap_.interpolate(0, 20);
                widgetAlpha = ColorAlpha(widgetAlpha, aswap_.interpolate(1.0f, 0.0f, Interpolation::Linear));
                if (aswap_.done())
                    swapWidget();
                break;
            case Transition::FadeIn:
                footerOffset = aswap_.interpolate(20, 0);
                widgetAlpha = ColorAlpha(widgetAlpha, aswap_.interpolate(0.0f, 1.0f, Interpolation::Linear));
                if (aswap_.done()) 
                    transition_ = Transition::None;
                break;
            default:
                break;
        }
        float headerOffset = 0;
        switch (header_) {
            case Transition::FadeOut:
                headerOffset = aheader_.interpolate(0, 20);
                if (aheader_.done()) {
                    header_ = Transition::None;
                    headerVisible_ = false;
                }
                break;
            case Transition::FadeIn:
                headerOffset = aheader_.interpolate(20, 0);
                if (aheader_.done())
                    header_ = Transition::None;
                break;
        }

        BeginDrawing();
        ClearBackground(ColorAlpha(BLACK, 0.0));
        if (backgroundOpacity_ == 255) {
            DrawTextureRec(backgroundCanvas_.texture, Rectangle{static_cast<float>(480 - backgroundSeam_),0,320,-240}, Vector2{0,0}, WHITE);
        } else {
            BeginBlendMode(BLEND_ADD_COLORS);
            DrawRectangle(0, 0, 320, 240, ColorAlpha(BLACK, backgroundOpacity_ / 255.0));
            EndBlendMode();
        }
        DrawTextureRec(widgetCanvas_.texture, Rectangle{0,0,320,-240}, Vector2{0,0}, widgetAlpha);
        if (headerVisible_)
            DrawTextureRec(headerCanvas_.texture, Rectangle{0,0,320,-240}, Vector2{0, -headerOffset}, WHITE);
        if (widget_ == nullptr || ! widget_->fullscreen())
            DrawTextureRec(footerCanvas_.texture, Rectangle{0,0,320,-240}, Vector2{0, footerOffset}, WHITE);
#if (defined RENDERING_STATS)
        auto astats = now();
        int i = 0;
        int x = fti_;
#if (defined RENDERING_STATS_DETAILED)
        while (i < 320) {
            DrawLine(i, 215, i, 215 - frames_[x].total, DARKGRAY);
            int y = 215;
            DrawLine(i, y, i, y - frames_[x].widget, RED);
            y -= frames_[x].widget;
            DrawLine(i, y, i, y - frames_[x].header, GREEN);
            y -= frames_[x].header;
            DrawLine(i, y, i, y - frames_[x].footer, BLUE);
            DrawPixel(i, 215 - frames_[x].delta, WHITE);
            x = (x + 1) % 320;
            ++i;
        }
#endif
        int last = fti_ == 0 ? 319 : fti_ - 1;
        DrawText(STR(frames_[last].total).c_str(), 270, 30, 16, DARKGRAY);
        DrawText(STR(frames_[last].background).c_str(), 270, 50, 16, WHITE);
        DrawText(STR(frames_[last].widget).c_str(), 270, 70, 16, RED);
        DrawText(STR(frames_[last].header).c_str(), 270, 90, 16, GREEN);
        DrawText(STR(frames_[last].footer).c_str(), 270, 110, 16, BLUE);
        DrawText(STR(frames_[last].render).c_str(), 270, 130, 16, DARKGRAY);
        DrawText(STR(frames_[last].stats).c_str(), 270, 150, 16, YELLOW);
        frames_[fti_].stats = asMillis(now() - astats);
#endif
#if (defined RENDERING_STATS_FPS)
        DrawText(STR(GetFPS()).c_str(), 290, 220, 20, WHITE);
#endif
        EndDrawing();
#if (defined RENDERING_STATS)
        frames_[fti_].render = asMillis(now() - t);
        frames_[fti_].total = asMillis(now() - t);
        fti_ = (fti_ + 1) % 320;
#endif
    } 
}

void Window::drawHeader() {
    //BeginTextureMode(buffer_);
    ClearBackground(ColorAlpha(BLACK, 0.0));

    // The heart, or time left
    DrawTextEx(headerFont_, "", 0, 0, 20, 1.0, DARKGRAY);
    BeginScissorMode(0, 10, 20, 20);
    DrawTextEx(headerFont_, "", 0, 0, 20, 1.0, PINK);
    EndScissorMode();

    BeginBlendMode(BLEND_ADD_COLORS);
    int x = 320;
    // charging and usb power indicator 
    if (rckid_->usb()) {
        x -= 10;
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, rckid_->charging() ? WHITE : GRAY);
    }
    // the battery level and percentage
    if (inHomeMenu_) {
        std::string pct = STR((rckid_->vBatt() - 330) << "%");
        x -= MeasureText(helpFont_, pct.c_str(), 16, 1.0).x + 5;
        DrawTextEx(helpFont_, pct.c_str(), x, 2, 16, 1, GRAY);
    }
    x -= 20;
    if (rckid_->vBatt() > 415)
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, GREEN);
    else if (rckid_->vBatt() > 390)
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, DARKGREEN);
    else if (rckid_->vBatt() > 375)
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, ORANGE);
    else if (rckid_->vBatt() > 340)    
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, RED);
    else 
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, RED);
    // volume & headphones
    if (rckid_->headphones()) {
        x -= 20;
        DrawTextEx(headerFont_, "󰋋", x, 0, 20, 1.0, WHITE);    
    }
    if (inHomeMenu_) {
        std::string vol = STR(rckid_->volume());
        x -= MeasureText(helpFont_, vol.c_str(), 16, 1.0).x + 5;
        DrawTextEx(helpFont_, vol.c_str(), x - 3, 2, 16, 1, GRAY);
    }
    x -= 20;
    if (rckid_->volume() == 0)
        DrawTextEx(headerFont_, "󰸈", x, 0, 20, 1.0, RED);
    else if (rckid_->volume() < 6)
        DrawTextEx(headerFont_, "󰕿", x, 0, 20, 1.0, BLUE);
    else if (rckid_->volume() < 12)
        DrawTextEx(headerFont_, "󰖀", x, 0, 20, 1.0, BLUE);
    else
        DrawTextEx(headerFont_, "󰕾", x, 0, 20, 1.0, ORANGE);
    // WiFi
    if (! rckid_->wifi() ) {
        x -= 20;
        DrawTextEx(headerFont_, "󰖪", x, 0, 20, 1.0, GRAY);
    } else {
        if (inHomeMenu_) {
            x -= MeasureText(helpFont_, rckid_->ssid().c_str(), 16, 1.0).x + 5;
            DrawTextEx(helpFont_, rckid_->ssid().c_str(), x - 3, 2, 16, 1, GRAY);
        }
        x -= 20;
        if (rckid_->wifiHotspot()) 
            DrawTextEx(headerFont_, "󱛁", x, 0, 20, 1.0, Color{0, 255, 255, 255});
        else
            DrawTextEx(headerFont_, "󰖩", x, 0, 20, 1.0, BLUE);
    }
    // NRF Radio
    if (rckid_->nrf()) {
        x -= 20;
        DrawTextEx(headerFont_, "󱄙", x, 0, 20, 1.0, GREEN);
    }

    EndBlendMode();
}

void Window::drawFooter() {
    ClearBackground(ColorAlpha(BLACK, 0.0));
    int  x = 0;
    BeginBlendMode(BLEND_ADD_COLORS);
    for (FooterItem const & item: footer_)
        x = item.draw(this, x, 220);
    EndBlendMode();
}