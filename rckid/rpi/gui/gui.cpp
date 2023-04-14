#include "platform/platform.h"

#include "carousel.h"
#include "debug_view.h"
#include "gauge.h"

#include "gui.h"


int FooterItem::draw(GUI * gui, int x, int y) const {
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
    DrawTextEx(gui->helpFont(), text_.c_str(), x, y + (20 - textSize_.y) / 2 , 16, 1.0, WHITE);
    x += textSize_.x + 5;
    return x;
}



GUI::GUI() {
    carousel_ = new Carousel{this};
    homeMenu_ = new Menu{this, {
        new Menu::Item{"Power Off", "assets/images/011-power-off.png"},
        new Menu::Item{"Airplane Mode", "assets/images/012-airplane-mode.png"},
        new WidgetItem{"Brightness", "assets/images/009-brightness.png", new Gauge{this}},
        new WidgetItem{"Volume", "assets/images/010-high-volume.png", new Gauge{this}},
        new Menu::Item{"WiFi", "assets/images/016-wifi.png"},
        new WidgetItem{"Debug", "assets/images/021-poo.png", new DebugView{this}},
    }};
}

void GUI::startRendering() {
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

void GUI::stopRendering() {
    rendering_ = false;
    for (GUIElement * e : elements_)
        e->onRenderingPaused();
    CloseWindow();
} 


void GUI::setWidget(Widget * widget) {
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

void GUI::setMenu(Menu * menu, size_t index) {
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

void GUI::back() {
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

void GUI::swapWidget() {
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

void GUI::processInputEvents(RCKid * rckid) {
    RCKid::Event e;
    while (true) {
        size_t events = rckid->getNextEvent(e);
        if (events == 0)
            break;
        switch (e.kind) {
            case RCKid::Event::Kind::Button: {
                switch (e.button.btn) {
                    case RCKid::Button::A:
                        btnA(e.button.state);
                        break;
                    case RCKid::Button::B:
                        btnB(e.button.state);
                        break;
                    case RCKid::Button::X:
                        btnX(e.button.state);
                        break;
                    case RCKid::Button::Y:
                        btnY(e.button.state);
                        break;
                    case RCKid::Button::L:
                        btnL(e.button.state);
                        break;
                    case RCKid::Button::R:
                        btnR(e.button.state);
                        break;
                    case RCKid::Button::Left:
                        dpadLeft(e.button.state);
                        break;
                    case RCKid::Button::Right:
                        dpadRight(e.button.state);
                        break;
                    case RCKid::Button::Up:
                        dpadUp(e.button.state);
                        break;
                    case RCKid::Button::Down:
                        dpadDown(e.button.state);
                        break;
                    case RCKid::Button::Select:
                        btnSelect(e.button.state);
                        break;
                    case RCKid::Button::Start:
                        btnStart(e.button.state);
                        break;
                    case RCKid::Button::Home:
                        btnHome(e.button.state);
                        break;
                    case RCKid::Button::VolumeUp:
                        btnVolUp(e.button.state);
                        break;
                    case RCKid::Button::VolumeDown:
                        btnVolDown(e.button.state);
                        break;
                    case RCKid::Button::Joy:
                        btnJoy(e.button.state);
                        break;
                }
            }
        }
        if (events == 1)
            break;
    }
}


void GUI::loop(RCKid * driver) {
    while (true) {
        processInputEvents(driver);
        if (rendering_)
            draw();
#if (defined ARCH_MOCK)
        //if (WindowShouldClose())
        //    break;
#endif
    }
}

void GUI::draw() {
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
            DrawRectangle(0, 0, GUI_WIDTH, GUI_HEIGHT, ColorAlpha(BLACK, swap_.interpolate(0.0f, 1.0f, Interpolation::Linear)));
            if (aend)
                swapWidget();
            break;
        case Transition::FadeIn:
            DrawRectangle(0, 0, GUI_WIDTH, GUI_HEIGHT, ColorAlpha(BLACK, swap_.interpolate(1.0f, 0.0f, Interpolation::Linear)));
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




void GUI::drawHeader() {
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

void GUI::drawFooter() {
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