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
    rckid_ = new RCKid{this};
    carousel_ = new Carousel{this};
    homeMenu_ = new Menu{this, {
        new Menu::Item{"Power Off", "assets/images/011-power-off.png"},
        new Menu::Item{"Airplane Mode", "assets/images/012-airplane-mode.png"},
        new WidgetItem{"Brightness", "assets/images/009-brightness.png", new Gauge{this, 
            [this](int value) { 
                rckid_->setBrightness(value); 
            }
        }},
        new WidgetItem{"Volume", "assets/images/010-high-volume.png", new Gauge{this, 
            [](int){}
        }},
        new Menu::Item{"WiFi", "assets/images/016-wifi.png"},
        new WidgetItem{"Debug", "assets/images/021-poo.png", new DebugView{this}},
    }};
}

void Window::startRendering() {
    InitWindow(320, 240, "RCKid");
    //InitWindow(640, 480, "RCKid");
    SetTargetFPS(60);
    rendering_ = true;
    if (! IsWindowReady())
        TraceLog(LOG_ERROR, "Unable to initialize window");
    helpFont_ = loadFont(HELP_FONT, 16);
    headerFont_ = loadFont(HELP_FONT, 20);
    menuFont_ = loadFont(MENU_FONT, MENU_FONT_SIZE);
    InitAudioDevice();
    lastDrawTime_ = GetTime();    
}

void Window::stopRendering() {
    // indicate the end of rendering
    rendering_ = false;
    // if we are not curently drawing, release all resources...
    if (!drawing_) {
        for (WindowElement * e : elements_)
            e->onRenderingPaused();
        fonts_.clear();
        CloseAudioDevice();
        CloseWindow();
    }
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

void Window::back() {
    if (nav_.size() == 0)
        return; 
    next_ = nav_.back();
    nav_.pop_back();
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
            if (carousel_->items() == homeMenu_)
                inHomeMenu_ = false;
            carousel_->items()->onBlur();
        }
    }
    if (next_.kind == NavigationItem::Kind::Menu) {
        widget_ = carousel_;
        carousel_->setItems(next_.menu(), next_.menuIndex());
        if (next_.menu() == homeMenu_)
            inHomeMenu_ = true;
    } else {
        widget_ = next_.widget();
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
        //if (WindowShouldClose())
        //    break;
#endif
        rckid_->loop();
        if (rendering_)
            draw();
    }
}

void Window::draw() {
    drawing_ = true;
    BeginDrawing();
    double t = GetTime();
    redrawDelta_ = static_cast<float>((t - lastDrawTime_) * 1000);
    bool aend = swap_.update(this);
    lastDrawTime_ = t;
    ClearBackground(ColorAlpha(BLACK, 0.0));
    // draw the widget
    if (widget_ != nullptr)
        widget_->draw();
    // do the fade in or out business
    switch (transition_) {
        case Transition::None:
            break;
        case Transition::FadeOut:
            DrawRectangle(0, 0, Window_WIDTH, Window_HEIGHT, ColorAlpha(BLACK, swap_.interpolate(0.0f, 1.0f, Interpolation::Linear)));
            if (aend)
                swapWidget();
            break;
        case Transition::FadeIn:
            DrawRectangle(0, 0, Window_WIDTH, Window_HEIGHT, ColorAlpha(BLACK, swap_.interpolate(1.0f, 0.0f, Interpolation::Linear)));
            if (aend) 
                transition_ = Transition::None;
            break;
        default: 
            UNREACHABLE;
    }
    // overlay the header and footer 
    drawHeader();
    if (widget_ == nullptr || ! widget_->fullscreen())
        drawFooter();
    // and we are done
    EndDrawing();
    drawing_ = false;
    // if rendering is off, it has been disabled while drawing, perform the resource release now
    if (rendering_ == false) 
        stopRendering();
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