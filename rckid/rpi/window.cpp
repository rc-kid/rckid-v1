
#include "platform/platform.h"
#include "utils/time.h"

#include "carousel.h"
#include "debug_view.h"
#include "gauge.h"

#include "window.h"

int FooterItem::draw(Canvas & canvas, int x, int y) const {
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
        case Control::UpDown:
            canvas.drawText(x + 5, y, "󰹺", canvas.accentColor(), canvas.defaultFont());
            x += 20;
            break;
        case Control::LeftRight:
            canvas.drawText(x + 5, y, "󰹴", canvas.accentColor(), canvas.defaultFont());
            x += 20;
            break;
        case Control::DPad:
            canvas.drawText(x + 5, y, "", canvas.accentColor(), canvas.defaultFont());
            x += 20;
            break;
    }
    canvas.drawText(x, y + 2, text_, WHITE, canvas.helpFont());
    x += textWidth_ + 5;
    return x;
}

Window & Window::create() {
    singleton_ = std::unique_ptr<Window>{new Window{}};
    return *singleton_;
}

Window::Window() {
    InitWindow(320, 240, "RCKid");
    SetTargetFPS(60);
    canvas_ = std::make_unique<Canvas>(320, 240);
    if (! IsWindowReady())
        TraceLog(LOG_ERROR, "Unable to initialize window");
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

void Window::showWidget(Widget * widget) {
    ASSERT(!widget->onNavStack());
    nav_.push_back(widget);
    // if we are the first widget proceed to enter directly 
    if (nav_.size() == 1) {
        enter(widget);
    // otherwise we must first fade the old widget, then start the new one
    } else {
        leave(nav_[nav_.size() - 2], /* navPop */ false);
        if (widget->fullscreen())
            hideHeader();
    }
}

void Window::showHomeMenu() {
    if (!homeMenu_->onNavStack())
        showWidget(homeMenu_);
}

void Window::back(size_t numWidgets) {
    ASSERT(numWidgets > 0);
    if (modal_ != nullptr) {
        --numWidgets;
        modal_->onBlur();
        modal_->onNavigationPop();
        modal_->onNavStack_ = false;
        modal_ = nullptr;
        // force widget repaint 
        nav_.back()->redraw_ = true;
    }
    // can't go back from the only widget in the navstack
    //if (nav_.size() < 2)
    //    return;
    // if we want to leave multiple widgets, unroll them while we can
    while (numWidgets > 0) {
        if (nav_.size() <= 1)
            break;
        leave(nav_.back(), /* navPop */ true);
        nav_.pop_back();
        --numWidgets;
    }
    // force the top widget redraw (if any) 
    if (!nav_.empty())
        nav_.back()->redraw_ = true;
}

void Window::showModal(Widget * widget) {
    ASSERT(modal_ == nullptr);
    ASSERT(!widget->onNavStack());
    if (modal_ != nullptr) {
        modal_->onBlur();
        modal_->onNavigationPop();
        modal_->onNavStack_ = false;
    }
    modal_ = widget;
    modal_->onNavigationPush();
    modal_->onNavStack_ = true;
    modal_->onFocus();
    modal_->setFooterHints();
    // force redraw of the modal window
    modal_->redraw_ = true;
}

void Window::enter(Widget * widget) {
    ASSERT(nav_.back() == widget);
    if (!widget->onNavStack_) {
        widget->onNavigationPush();
        widget->onNavStack_ = true;
    }
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

void Window::leave(Widget * widget, bool navPop) {
    widget->onBlur();
    if (navPop) {
        widget->onNavigationPop();
        widget->onNavStack_ = false;
    }
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
                    [this](comms::Mode) {
                        redrawHeader_ = true;
                    },
                    [this](comms::PowerStatus) {
                        redrawHeader_ = true;
                        // TODO
                    }, 
                    [this](SecondTick) {
                        // TODO do we want to do anything here? 
                    }, 
                    [this](AlarmEvent) {
                        // TODO
                    },
                    [this](StateChangeEvent) {
                        redrawHeader_ = true;
                    },
                    [this](Hearts h) {
                        redrawHeader_ = true;
                        // if there are no more hearts, unroll any widgets that cause their counting
                        if (h.value == 0) {
                            while (rckid().heartsCounterEnabled() && nav_.size() > 1)
                                back();
                            // and display the warning
                            // TODO
                        } else if (h.value < HEARTS_LOW_THRESHOLD && !headerVisible_) {
                            showHeader();
                        }
                    }, 
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
                    [this, w](JoyEvent et) {
                        w->joy(et.h, et.v);
                    },
                    [this, w](AccelEvent ea) {
                        w->accel(ea.h, ea.v);
                    }, 
                    [this](HeadphonesEvent) {
                        redrawHeader_ = true;
                    },
                    // simply process the recorded data - we know the function must exist since it must be supplied very time we start recording
                    [this, w](RecordingEvent e) {
                        w->audioRecorded(e);
                    },
                    [this, w](NRFPacketEvent e) {
                        w->nrfPacketReceived(e);
                    },
                    [this, w](NRFTxEvent e) {
                        w->nrfTxDone();
                    },
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
    canvas_->update();

    bool redraw = false;

    // Start by rendering the background. 
    //
    // TODO this can be simplified by prerendering the background into twice large texture and then only copy the parts of it as needed according to the seam elliminating the need for the 2 texture draws when background changes
#if (defined RENDERING_STATS)
    frames_[fti_].delta = redrawDelta_;
    Timepoint tt = now();
#endif
    if (redrawBackground_ || FORCE_FULL_REDRAW) {
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
    Widget * w = (nav_.empty() || tSwap_ != Transition::None) ? nullptr : nav_.back();
    if (w == nullptr && tSwap_ == Transition::FadeIn)
        w = nav_.back();
    if (w != nullptr) {
        bool wredraw = w->redraw_ || (modal_ != nullptr && modal_->redraw_);
        w->tick();
        if (wredraw || FORCE_FULL_REDRAW) {
            redraw = true;
            BeginTextureMode(widgetCanvas_);
            ClearBackground(ColorAlpha(BLACK, 0.0));
            BeginBlendMode(BLEND_ADD_COLORS);
            canvas_->resetDefaults();
            w->draw(*canvas_);
            // draw the modal widget, if any
            if (modal_) {
                canvas_->resetDefaults();
                modal_->draw(*canvas_);
            }
            // and we are done
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
    if (redrawHeader_ || FORCE_FULL_REDRAW) {
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
    if (redrawFooter_ || FORCE_FULL_REDRAW) {
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
    using namespace comms;
    //BeginTextureMode(buffer_);
    ClearBackground(ColorAlpha(BLACK, 0.0));
    canvas_->setDefaultFont();

    // draw the hearts. If we have enough left, just the red hearts is displayed, regardless of the value. If we have fewer hearts, then the red outline, filled with appropriate pink is displayed and unless the current widget is in fullscreen mode, its accompanied by a text countdown (h m s, one of). If in fullscreen mode then just the hearts are shown and the rest of the header is actually not rendered at all since for full screen widgets with hidden header, when we get too few hearts, the header is always shown 
    ::Color hOutline = rckid().heartsCounterEnabled() ? RED : GRAY;
    ::Color hFill = rckid().heartsCounterEnabled() ? PINK : LIGHTGRAY;
    if (rckid().hearts() >= 3 * HEART_SIZE_SECONDS) {
        canvas_->drawText(0, 0, "  ", hOutline);
    } else {
        // int x = canvas_->textWidth("   ");
        BeginScissorMode(0, 0, 53 * rckid().hearts() / (3 * HEART_SIZE_SECONDS), 20);
        canvas_->drawText(0, 0, "  ", hFill);
        EndScissorMode();
        canvas_->drawText(0, 0, "  ", hOutline);
        // TODO check if we should display the text too
        BeginBlendMode(BLEND_ADD_COLORS);
        std::string remaining = toHorMorS(rckid().hearts());
        canvas_->drawText(56, 2, remaining, WHITE, canvas_->helpFont());
        EndBlendMode();
    }


    ExtendedState state{rckid().extendedState()};

    
    BeginBlendMode(BLEND_ADD_COLORS);
    
    /*
    canvas_->drawText(0, 0, "  ", RED);
    BeginScissorMode(0, 10, 20, 20);
    canvas_->drawText(0, 0, "", PINK);
    EndScissorMode();
    */

    // The heart, or time left
    /*
    canvas_->drawText(0, 0, "", DARKGRAY);
    BeginScissorMode(0, 10, 20, 20);
    canvas_->drawText(0, 0, "", PINK);
    EndScissorMode();
    */

    int x = 320;
    // power status (battery level, charging, etc.)
    switch (state.status.powerStatus()) {
        case PowerStatus::LowBattery:
            x -= 10;
            canvas_->drawText(x, 0, "!", RED);
            // fallthorugh to battery
        case PowerStatus::Battery:
            drawBatteryGauge(x, state.einfo.vBatt());
            break;
        case PowerStatus::Charging:
            x -= 10;
            canvas_->drawText(x, 0, "", BLUE);
            drawBatteryGauge(x, state.einfo.vBatt());
            break;            
        case PowerStatus::USB:
            x -= 10;
            canvas_->drawText(x, 0, "", GREEN);
            drawBatteryGauge(x, state.einfo.vBatt()); 
            break;
    }
    // volume & headphones
    if (rckid().headphones()) {
        x -= 20;
        canvas_->drawText(x, 0, "󰋋", WHITE);    
    }
    if (homeMenu_->onNavStack()) {
        std::string vol = STR(rckid().volume());
        x -= canvas_->textWidth(vol, canvas_->helpFont()) + 5;
        canvas_->drawText(x - 3, 2, vol, GRAY, canvas_->helpFont());
    }
    x -= 20;
    if (rckid().volume() == 0)
        canvas_->drawText(x, 0, "󰸈", RED);
    else if (rckid().volume() < 6)
        canvas_->drawText(x, 0, "󰕿", BLUE);
    else if (rckid().volume() < 12)
        canvas_->drawText(x, 0, "󰖀", BLUE);
    else
        canvas_->drawText(x, 0, "󰕾", ORANGE);
    // WiFi
    /* TODO
    if (! rckid().wifi() ) {
        x -= 20;
        canvas_->drawText(x, 0, "󰖪", GRAY);
    } else {
        if (homeMenu_->onNavStack()) {
            x -= canvas_->textWidth(rckid().ssid(), canvas_->helpFont()) + 5;
            canvas_->drawText(x -3, 2, rckid().ssid(), GRAY, canvas_->helpFont());
        }
        x -= 20;
        if (rckid().wifiHotspot()) 
            canvas_->drawText(x, 0, "󱛁", Color{0, 255, 255, 255});
        else
            canvas_->drawText(x, 0, "󰖩", BLUE);
    }
    */
    // NRF Radio
    switch (rckid().nrfState()) {
        case NRFState::Error:
            x -= 20;
            canvas_->drawText(x, 0, "󱄙", RED);
            break;
        case NRFState::Rx:
            x -= 20;
            canvas_->drawText(x, 0, "󱄙", GREEN);
            break;
        case NRFState::Tx:
            x -= 20;
            canvas_->drawText(x, 0, "󱄙", BLUE);
            break;
    }
    EndBlendMode();

}

void Window::drawBatteryGauge(int & x, uint16_t vbatt) {
    // since the valid battery level is anything from 3.3 to 4.3 volts, getting percentage is super easy
    size_t vpct = vbatt <= 330 ? 0 : (vbatt >= 420) ? 100 : (vbatt - 330) * 100 / 90;
    // the battery level and percentage
    if (homeMenu_->onNavStack()) {
        std::string pct = STR(vpct << "%");
        x -= canvas_->textWidth(pct, canvas_->helpFont()) + 5;
        canvas_->drawText(x, 2, pct, GRAY, canvas_->helpFont());
    }
    x -= 23;
    if (vpct >= 80)
        canvas_->drawText(x, 0, "", GREEN);
    else if (vpct >= 60)
        canvas_->drawText(x, 0, "", DARKGREEN);
    else if (vpct >= 40)
        canvas_->drawText(x, 0, "", ORANGE);
    else if (vpct >= 20)    
        canvas_->drawText(x, 0, "", RED);
    else 
        canvas_->drawText(x, 0, "", RED);
}

void Window::drawFooter() {
    ClearBackground(ColorAlpha(BLACK, 0.0));
    int  x = 0;
    BeginBlendMode(BLEND_ADD_COLORS);
    for (FooterItem const & item: footer_)
        x = item.draw(*canvas_, x, 220);
    EndBlendMode();
}