
#include "platform/platform.h"

#include "carousel.h"
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
        case Control::Select:
            DrawRectangleRounded(RECT(x + 7, y + 4, 6, 12), 1, 6, WHITE);
            x += 20;
            break;
    }
    DrawTextEx(window->helpFont(), text_.c_str(), x, y + (20 - textSize_.y) / 2 , 16, 1.0, WHITE);
    x += textSize_.x + 5;
    return x;
}

Window & Window::create() {
    singleton_ = std::unique_ptr<Window>{new Window{}};
    return *singleton_;
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
    modalCanvas_ = LoadRenderTexture(320, 240);
    headerCanvas_ = LoadRenderTexture(320, 240);
    footerCanvas_ = LoadRenderTexture(320, 240);
    BeginTextureMode(backgroundCanvas_);
    ClearBackground(ColorAlpha(BLACK, 0.0));
    DrawRectangle(0,0,640,240, BLACK);
    DrawTexture(background_, 0, 0, ColorAlpha(WHITE, BACKGROUND_OPACITY));
    DrawTexture(background_, 320, 0, ColorAlpha(WHITE, BACKGROUND_OPACITY));
    EndTextureMode();

    InitAudioDevice();

    homeMenu_ = new Carousel{new Carousel::Menu{"", "", {
        new Carousel::Item{"Exit", "assets/images/011-power-off.png", [](){
            ::exit(0);
        }},
        new Carousel::Item{"Power Off", "assets/images/011-power-off.png", [](){
            rckid().powerOff();
        }},
        new Carousel::Item{"Airplane Mode", "assets/images/012-airplane-mode.png", [](){
            // TODO
        }},
        new Carousel::Item{"Brightness", "assets/images/009-brightness.png", [](){
            static Gauge gauge{"assets/images/009-brightness.png", 0, 255, 16,
                [](int value) { 
                    rckid().setBrightness(value); 
                },
                [](Gauge * g) {
                    g->setValue(rckid().brightness());
                }
            };
            window().showWidget(&gauge);
        }}, 
        new Carousel::Item{"Volume", "assets/images/010-high-volume.png", [](){
            static Gauge gauge{"assets/images/010-high-volume.png", 0, 100, 10,
                [](int value){
                    rckid().setVolume(value);
                },
                [](Gauge * g) {
                    g->setValue(rckid().volume());
                }
            };
            window().showWidget(&gauge);
        }},
        new Carousel::Item{"WiFi", "assets/images/016-wifi.png", [](){
            // TODO
        }},
        new Carousel::Item{"Debug", "assets/images/021-poo.png", [](){
            static DebugView dbgView{};
            window().showWidget(&dbgView);
        }},

    }}};
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

void Window::showWidget(Widget * widget) {
    nav_.push_back(widget);
    // if we are the first widget proceed to enter directly 
    if (nav_.size() == 1) {
        enter(widget);
    // otherwise we must first fade the old widget, then start the new one
    } else {
        leave(nav_[nav_.size() - 2]);
        if (widget->fullscreen())
            hideHeader();
    }
}

void Window::showHomeMenu() {
    if (!homeMenu_->onNavStack())
        showWidget(homeMenu_);
}

void Window::back(size_t numWidgets) {
    // can't go back from the only widget in the navstack
    if (nav_.size() < 2)
        return;
    // if we want to leave multiple widgets, unroll them while we can
    while (numWidgets > 0) {
        if (nav_.size() <= 1)
            break;
        leave(nav_.back());
        nav_.pop_back();
        --numWidgets;
    }
}

void Window::showModal(Widget * widget) {
    modalNav_.push_back(widget); 
}

void Window::enter(Widget * widget) {
    ASSERT(nav_.back() == widget);
    ASSERT(!widget->onNavStack());
    widget->onNavigationPush();
    widget->onNavStack_ = true;
    widget->onFocus();
    widget->setFooterHints();
    // set the swap transition to fade the widget in and require its redraw
    tSwap_ = Transition::FadeIn;
    aSwap_.start();
    widget->redraw_ = true;
    // force header redraw when swapping widget, just to be sure
    redrawHeader_ = true;
    // if the widget is not full screen, show the header and the footer as well
    if (!widget->fullscreen()) {
        showFooter();
        showHeader();
    }
}

void Window::leave(Widget * widget) {
    widget->onBlur();
    widget->onNavigationPop();
    widget->onNavStack_ = false;
    // set the transition 
    tSwap_ = Transition::FadeOut;
    aSwap_.start();
    // hide the footer with the widget as well (enter will show it again)
    hideFooter();
}

void Window::loop() {
    Event e;
    while (true) {
#if (defined ARCH_MOCK)
        if (WindowShouldClose())
            break;
#endif
        // process any RCkid's evenys
        while (true) {
            auto event = rckid().nextEvent();
            if (!event.has_value())
                break;
            Widget * w = activeWidget();
            if (w) {
                std::visit(overloaded{
                    [this, w](ButtonEvent eb) {
                        TraceLog(LOG_DEBUG, STR("Button state change. Button: " << (int)eb.btn << ", state: " << eb.state));
                        switch (eb.btn) {
                            case Button::A:
                                w->btnA(eb.state);
                                break;
                            case Button::B:
                                w->btnB(eb.state);
                                break;
                            case Button::X:
                                w->btnX(eb.state);
                                break;
                            case Button::Y:
                                w->btnY(eb.state);
                                break;
                            case Button::L:
                                w->btnL(eb.state);
                                break;
                            case Button::R:
                                w->btnR(eb.state);
                                break;
                            case Button::Left:
                                w->dpadLeft(eb.state);
                                break;
                            case Button::Right:
                                w->dpadRight(eb.state);
                                break;
                            case Button::Up:
                                w->dpadUp(eb.state);
                                break;
                            case Button::Down:
                                w->dpadDown(eb.state);
                                break;
                            case Button::Select:
                                w->btnSelect(eb.state);
                                break;
                            case Button::Start:
                                w->btnStart(eb.state);
                                break;
                            case Button::Home:
                                w->btnHome(eb.state);
                                break;
                            case Button::Joy:
                                w->btnJoy(eb.state);
                                break;
                        }
                    }, 
                    [this, w](ThumbEvent et) {
                        w->joy(et.x, et.y);
                    },
                    [this, w](AccelEvent ea) {
                        w->accel(ea.x, ea.y);
                    }, 
                    // simply process the recorded data - we know the function must exist since it must be supplied very time we start recording
                    [this, w](RecordingEvent e) {
                        w->audioRecorded(e);
                    },
                    [this, w](NRFPacketEvent e) {
                        w->nrfPacketReceived(e);
                    },
                    [this, w](NRFTxIrq e) {
                        w->nrfTxCallback(! e.nrfStatus.txDataFailIrq());
                    },
                    // don't do anything for other events
                    [this](auto e) { }

                }, event.value());
            }    
        }
        // when all rckid's events are processed, try drawing
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
    aSwap_.update();
    aHeader_.update();
    aFooter_.update();

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
    Widget * w = activeWidget();
    if (w == nullptr && tSwap_ == Transition::FadeIn)
        w = nav_.back();
    if (w != nullptr) {
        w->tick();
        if (w->redraw_) {
            redraw = true;
            BeginTextureMode(widgetCanvas_);
            ClearBackground(ColorAlpha(BLACK, 0.0));
            BeginBlendMode(BLEND_ADD_COLORS);
            w->draw();
            EndBlendMode();
            EndTextureMode();
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
    if (rckid().statusChanged()) 
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
    redraw = redraw || tSwap_ != Transition::None || tHeader_ != Transition::None || tFooter_ != Transition::None;

    // finally, piece together the actual frame
    if (redraw) {
#if (defined RENDERING_STATS)
        tt = now();
#endif
        // determine if we are drawing header or footer as those can change in the transition switches below
        bool drawHeader = headerVisible_ || aHeader_.running();
        bool drawFooter = footerVisible_ || aFooter_.running();
        ::Color widgetAlpha = WHITE;
        float footerOffset = 0;
        switch (tFooter_) {
            case Transition::FadeIn:
                footerOffset = aFooter_.interpolate(20, 0);
                if (aFooter_.done()) 
                    tFooter_ = Transition::None;
                break;
            case Transition::FadeOut:
                footerOffset = aFooter_.interpolate(0, 20);
                if (aFooter_.done())
                    tFooter_ = Transition::None;
                break;
            default:
                break;
        }
        float headerOffset = 0;
        switch (tHeader_) {
            case Transition::FadeIn:
                headerOffset = aHeader_.interpolate(20, 0);
                if (aHeader_.done())
                    tHeader_ = Transition::None;
                break;
            case Transition::FadeOut:
                headerOffset = aHeader_.interpolate(0, 20);
                if (aHeader_.done())
                    tHeader_ = Transition::None;
                break;
        }
        switch (tSwap_) {
            case Transition::FadeIn:
                widgetAlpha = ColorAlpha(widgetAlpha, aSwap_.interpolate(0.0f, 1.0f, Interpolation::Linear));
                if (aSwap_.done())
                    tSwap_ = Transition::None;
                break;
            case Transition::FadeOut:
                widgetAlpha = ColorAlpha(widgetAlpha, aSwap_.interpolate(1.0f, 0.0f, Interpolation::Linear));
                if (aSwap_.done())
                    enter(nav_.back());
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
        if (drawHeader)
            DrawTextureRec(headerCanvas_.texture, Rectangle{0,0,320,-240}, Vector2{0, -headerOffset}, WHITE);
        if (drawFooter)
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
    } else {
        platform::cpu::delayMs(16); // to maintain 60fps illusion
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
    if (rckid().usb()) {
        x -= 10;
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, rckid().charging() ? WHITE : GRAY);
    }
    // the battery level and percentage
    if (homeMenu_->onNavStack()) {
        std::string pct = STR((rckid().vBatt() - 330) << "%");
        x -= MeasureText(helpFont_, pct.c_str(), 16, 1.0).x + 5;
        DrawTextEx(helpFont_, pct.c_str(), x, 2, 16, 1, GRAY);
    }
    x -= 20;
    if (rckid().vBatt() > 415)
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, GREEN);
    else if (rckid().vBatt() > 390)
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, DARKGREEN);
    else if (rckid().vBatt() > 375)
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, ORANGE);
    else if (rckid().vBatt() > 340)    
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, RED);
    else 
        DrawTextEx(headerFont_, "", x, 0, 20, 1.0, RED);
    // volume & headphones
    if (rckid().headphones()) {
        x -= 20;
        DrawTextEx(headerFont_, "󰋋", x, 0, 20, 1.0, WHITE);    
    }
    if (homeMenu_->onNavStack()) {
        std::string vol = STR(rckid().volume());
        x -= MeasureText(helpFont_, vol.c_str(), 16, 1.0).x + 5;
        DrawTextEx(helpFont_, vol.c_str(), x - 3, 2, 16, 1, GRAY);
    }
    x -= 20;
    if (rckid().volume() == 0)
        DrawTextEx(headerFont_, "󰸈", x, 0, 20, 1.0, RED);
    else if (rckid().volume() < 6)
        DrawTextEx(headerFont_, "󰕿", x, 0, 20, 1.0, BLUE);
    else if (rckid().volume() < 12)
        DrawTextEx(headerFont_, "󰖀", x, 0, 20, 1.0, BLUE);
    else
        DrawTextEx(headerFont_, "󰕾", x, 0, 20, 1.0, ORANGE);
    // WiFi
    if (! rckid().wifi() ) {
        x -= 20;
        DrawTextEx(headerFont_, "󰖪", x, 0, 20, 1.0, GRAY);
    } else {
        if (homeMenu_->onNavStack()) {
            x -= MeasureText(helpFont_, rckid().ssid().c_str(), 16, 1.0).x + 5;
            DrawTextEx(helpFont_, rckid().ssid().c_str(), x - 3, 2, 16, 1, GRAY);
        }
        x -= 20;
        if (rckid().wifiHotspot()) 
            DrawTextEx(headerFont_, "󱛁", x, 0, 20, 1.0, Color{0, 255, 255, 255});
        else
            DrawTextEx(headerFont_, "󰖩", x, 0, 20, 1.0, BLUE);
    }
    // NRF Radio
    switch (rckid().nrfState()) {
        case NRFState::Error:
            x -= 20;
            DrawTextEx(headerFont_, "󱄙", x, 0, 20, 1.0, RED);
            break;
        case NRFState::Receiver:
            x -= 20;
            DrawTextEx(headerFont_, "󱄙", x, 0, 20, 1.0, GREEN);
            break;
        case NRFState::Transmitting:
            x -= 20;
            DrawTextEx(headerFont_, "󱄙", x, 0, 20, 1.0, BLUE);
            break;
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