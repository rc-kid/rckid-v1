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
    canvas_ = LoadRenderTexture(320, 240);

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
        new WidgetItem{"Brightness", "assets/images/009-brightness.png", new Gauge{this, "Brightness", 0, 255, 16, 
            [this](int value) { 
                rckid_->setBrightness(value); 
            },
            [this](Gauge * g) {
                g->setValue(rckid_->brightness());
            }
        }},
        new WidgetItem{"Volume", "assets/images/010-high-volume.png", new Gauge{this, "Volume", 0, 100, 10,
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
    lastDrawTime_ = GetTime();    
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
        swap_.start();
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
        swap_.start();
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
    if (widget_ == nullptr) {
        swapWidget();
    } else {
        transition_ = Transition::FadeOut;
        swap_.start();
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
    swap_.start();
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

void Window::draw() {
    double t = GetTime();
    redrawDelta_ = static_cast<float>((t - lastDrawTime_) * 1000);
    bool aend = swap_.update(this);
    lastDrawTime_ = t;
    // draw the widget which we do on the render texture
    BeginTextureMode(canvas_);
    ClearBackground(ColorAlpha(BLACK, 0.0));
    if (widget_ != nullptr)
        widget_->draw();
    // do the fade in or out business
    EndTextureMode();

    BeginDrawing();
    ClearBackground(ColorAlpha(BLACK, 0.0));
    if (backgroundEnabled_) {
        // TODO why was this there????
        //DrawRectangle(0,0,320,240, BLACK);
        //BeginBlendMode(1);
        DrawTexture(background_, backgroundSeam_ - 320, 0, ColorAlpha(WHITE, 0.3));
        DrawTexture(background_, backgroundSeam_, 0, ColorAlpha(WHITE, 0.3));
        //EndBlendMode();
    }
    ::Color c = WHITE;
    switch (transition_) {
        case Transition::None:
            break;
        case Transition::FadeOut:
            c = ColorAlpha(c, swap_.interpolate(1.0f, 0.0f, Interpolation::Linear));
            if (aend)
                swapWidget();
            break;
        case Transition::FadeIn:
            c = ColorAlpha(c, swap_.interpolate(0.0f, 1.0f, Interpolation::Linear));
            if (aend) 
                transition_ = Transition::None;
            break;
        default: 
            UNREACHABLE;
    }
    DrawTextureRec(canvas_.texture, Rectangle{0,0,320,-240}, Vector2{0,0}, c);
    // overlay the header and footer 
    drawHeader();
    if (widget_ == nullptr || ! widget_->fullscreen())
        drawFooter();
    // and we are done
    EndDrawing();
}

void Window::drawHeader() {
    //DrawFPS(0,0);

    // battery voltage
    //DrawRectangle(0, 0, 320, 20, DARKGRAY);


    //DrawTextEx(menuFont_, "RCGirl", 100, 160, 64, 1.0, WHITE);
    DrawTextEx(headerFont_, "  ", 0, 0, 20, 1.0, PINK);
    DrawRectangle(20, 0, 40, 20, BLACK);
    DrawTextEx(headerFont_, "  ", 0, 0, 20, 1.0, RED);



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
        DrawTextEx(headerFont_, "󰸈", x, 0, 20, 1.0, BLUE);
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
}

void Window::drawFooter() {
    int y = 220;
    switch (transition_) {
        case Transition::None:
            break;
        case Transition::FadeOut:
            y += swap_.interpolate(0, 20);
            break;
        case Transition::FadeIn:
            y += swap_.interpolate(20, 0);
            break;
    }
    int  x = 0;
    for (FooterItem const & item: footer_)
        x = item.draw(this, x, y);
    DrawTextEx(helpFont_, STR(GetFPS()).c_str(), 300, 222, 16, 1.0, WHITE);


}