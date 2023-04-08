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
    if (item.kind == NavigationItem::Kind::Menu && widget_ == carousel_) {
        carousel_->setItems(item.menu(), item.menuIndex());
    } else {
        // TODO 
    }    
}

void GUI::processInputEvents() {
#if (defined ARCH_MOCK)
#define CHECK_KEY(KEY, FN) if (IsKeyPressed(KEY)) FN(true); if (IsKeyReleased(KEY)) FN(false);
    CHECK_KEY(KEY_A, btnA);
    CHECK_KEY(KEY_B, btnB);
    CHECK_KEY(KEY_X, btnX);
    CHECK_KEY(KEY_Y, btnY);
    CHECK_KEY(KEY_L, btnL);
    CHECK_KEY(KEY_R, btnR);
    CHECK_KEY(KEY_ENTER, btnSelect);
    CHECK_KEY(KEY_SPACE, btnStart);
    CHECK_KEY(KEY_D, btnDpad);
    CHECK_KEY(KEY_LEFT, dpadLeft);
    CHECK_KEY(KEY_RIGHT, dpadRight);
    CHECK_KEY(KEY_UP, dpadUp);
    CHECK_KEY(KEY_DOWN, dpadDown);
    CHECK_KEY(KEY_COMMA, btnVolDown);
    CHECK_KEY(KEY_PERIOD, btnVolUp);
    CHECK_KEY(KEY_H, btnHome);
#undef CHECK_KEY
#endif
    // TODO actually check the gamepad stuff
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