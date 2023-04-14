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
    }
    DrawTextEx(window->helpFont(), text_.c_str(), x, y + (20 - textSize_.y) / 2 , 16, 1.0, WHITE);
    x += textSize_.x + 5;
    return x;
}



Window::Window() {
    carousel_ = new Carousel{this};
    homeMenu_ = new Menu{this, {
        new Menu::Item{"Power Off", "assets/images/011-power-off.png"},
        new Menu::Item{"Airplane Mode", "assets/images/012-airplane-mode.png"},
        new WidgetItem{"Brightness", "assets/images/009-brightness.png", new Gauge{this}},
        new WidgetItem{"Volume", "assets/images/010-high-volume.png", new Gauge{this}},
        new Menu::Item{"WiFi", "assets/images/016-wifi.png"},
        new WidgetItem{"Debug", "assets/images/021-poo.png", new DebugView{this}},
    }};
    rckid_ = new RCKid{this};
}

void Window::startRendering() {
    InitWindow(320, 240, "RCKid");
    SetTargetFPS(60);
    rendering_ = true;
    if (! IsWindowReady())
        TraceLog(LOG_ERROR, "Unable to initialize window");
    helpFont_ = LoadFontEx("assets/fonts/IosevkaNF.ttf", 16, const_cast<int*>(GLYPHS), sizeof(GLYPHS)/ sizeof(int));
    headerFont_ = LoadFontEx("assets/fonts/IosevkaNF.ttf", 20, const_cast<int*>(GLYPHS), sizeof(GLYPHS)/ sizeof(int));
    menuFont_ = LoadFontEx("assets/fonts/OpenDyslexicNF.otf", MENU_FONT_SIZE, const_cast<int*>(GLYPHS), sizeof(GLYPHS) / sizeof(int));
    lastDrawTime_ = GetTime();    
}

void Window::stopRendering() {
    rendering_ = false;
    for (WindowElement * e : elements_)
        e->onRenderingPaused();
    CloseWindow();
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
        if (widget_ == carousel_ && carousel_->items() == homeMenu_)
            inHomeMenu_ = false;
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
        if (rendering_) {
            auto event = events_.receive();
            if (!event.has_value()) {
                draw();
                continue;
            } else {
                e = std::move(event.value());
            }
        } else {
            e = events_.waitReceive();
        }
        switch (e.kind) {
            case Event::Kind::Button: {
                ButtonEvent &eb = e.button();
                switch (eb.btn) {
                    case Button::A:
                        btnA(eb.state);
                        break;
                    case Button::B:
                        btnB(eb.state);
                        break;
                    case Button::X:
                        btnX(eb.state);
                        break;
                    case Button::Y:
                        btnY(eb.state);
                        break;
                    case Button::L:
                        btnL(eb.state);
                        break;
                    case Button::R:
                        btnR(eb.state);
                        break;
                    case Button::Left:
                        dpadLeft(eb.state);
                        break;
                    case Button::Right:
                        dpadRight(eb.state);
                        break;
                    case Button::Up:
                        dpadUp(eb.state);
                        break;
                    case Button::Down:
                        dpadDown(eb.state);
                        break;
                    case Button::Select:
                        btnSelect(eb.state);
                        break;
                    case Button::Start:
                        btnStart(eb.state);
                        break;
                    case Button::Home:
                        btnHome(eb.state);
                        break;
                    case Button::VolumeUp:
                        btnVolUp(eb.state);
                        break;
                    case Button::VolumeDown:
                        btnVolDown(eb.state);
                        break;
                    case Button::Joy:
                        btnJoy(eb.state);
                        break;
                }
            }
        }
    }
}

void Window::draw() {
    BeginDrawing();
    double t = GetTime();
    redrawDelta_ = static_cast<float>((t - lastDrawTime_) * 1000);
    bool aend = swap_.update(this);
    lastDrawTime_ = t;
    ClearBackground(BLACK);
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
}




void Window::drawHeader() {
    //DrawFPS(0,0);

    // battery voltage
    //DrawRectangle(0, 0, 320, 20, DARKGRAY);


    //DrawTextEx(menuFont_, "RCGirl", 100, 160, 64, 1.0, WHITE);
    DrawTextEx(headerFont_, "  ", 0, 0, 20, 1.0, PINK);
    DrawRectangle(20, 0, 40, 20, BLACK);
    DrawTextEx(headerFont_, "  ", 0, 0, 20, 1.0, RED);


    DrawTextEx(headerFont_, "󰸈 󰕿 󰖀 󰕾", 90, 0, 20, 1.0, BLUE);
    DrawTextEx(helpFont_, STR(GetFPS()).c_str(), 70, 2, 16, 1.0, WHITE);
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
/*        DrawCircle(x + 10, y + 10, 6, item.color);
        x += 20; 
        Vector2 size = MeasureTextEx(helpFont_, item.text.c_str(), 16, 1.0);
        DrawTextEx(helpFont_, item.text.c_str(), x, y + (20 - size.y) / 2 , 16, 1.0, WHITE);
        x += size.x + 5;
    } */
}