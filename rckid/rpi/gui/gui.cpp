#include "platform/platform.h"

#include "carousel.h"
#include "gui.h"

void Widget::btnB(bool state) {
    if (state)
        gui_->back();
}

void Widget::btnHome(bool state) {
    if (state)
        gui_->setMenu(gui_->homeMenu_, 0);
}

GUI::GUI():
    helpFont_{LoadFont("assets/fonts/Iosevka.ttf")},
    menuFont_{LoadFont("assets/fonts/OpenDyslexic.otf")},
    lastDrawTime_{GetTime()} {
    carousel_ = new Carousel{this};
    homeMenu_ = new Menu{
        Menu::Item{"Power Off", "assets/images/011-power-off.png"},
        Menu::Item{"Airplane Mode", "assets/images/012-airplane-mode.png"},
        Menu::Item{"Baby Monitor", "assets/images/006-baby-crib.png"},
        Menu::Item{"Settings", "assets/images/013-settings.png"},
    };
}

void GUI::setWidget(Widget * widget) {
    if (widget_ == carousel_)
        nav_.push_back(NavigationItem(carousel_->items(), carousel_->index()));
    else if (widget_ != nullptr) 
        nav_.push_back(NavigationItem(widget_));
    widget_ = widget;
}

void GUI::setMenu(Menu * menu, size_t index) {
    if (menu == homeMenu_) {
        if (inHomeMenu_)
            return;
        inHomeMenu_ = true;
    }
    if (widget_ == carousel_)
        nav_.push_back(NavigationItem(carousel_->items(), carousel_->index()));
    else if (widget_ != nullptr) 
        nav_.push_back(NavigationItem(widget_));
    widget_ = carousel_;
    carousel_->setItems(menu, index);
}

void GUI::back() {
    if (nav_.size() == 0)
        return; 
    NavigationItem item = nav_.back();
    nav_.pop_back();
    // check if we are leaving the home menu
    if (widget_ == carousel_ && carousel_->items() == homeMenu_)
        inHomeMenu_ = false;
    if (item.kind == NavigationItem::Kind::Menu && widget_ == carousel_) {
        carousel_->setItems(item.menu(), item.menuIndex());
    } else {
        // TODO 
    }    
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

void GUI::draw() {
    double t = GetTime();
    double delta = (t - lastDrawTime_) * 1000;
    lastDrawTime_ = t;
    BeginDrawing();
    ClearBackground(BLACK);
    // draw the widget
    if (widget_ != nullptr)
        widget_->draw(delta);
    // overlay the header and footer 
    drawHeader();
    drawFooter();
    EndDrawing();
}




void GUI::drawHeader() {
    //DrawFPS(0,0);

    // battery voltage

    //DrawTextEx(menuFont_, "RCGirl", 100, 160, 64, 1.0, WHITE);
    DrawTextEx(helpFont_, STR(GetFPS()).c_str(), 0, 0, 16, 1.0, WHITE);
}

void GUI::drawFooter() {
    float x = 0;
    for (FooterItem const & item: footer_) {
        DrawCircle(x + 10, 230, 6, item.color);
        x += 20; 
        Vector2 size = MeasureTextEx(helpFont_, item.text.c_str(), 16, 1.0);
        DrawTextEx(helpFont_, item.text.c_str(), x, 220 + (20 - size.y) / 2 , 16, 1.0, WHITE);
        x += size.x + 5;
    }
}