#include "platform/platform.h"

#include "gui.h"

GUI::GUI():
    helpFont_{LoadFont("assets/fonts/Iosevka.ttf")},
    menuFont_{LoadFont("assets/fonts/OpenDyslexic.otf")},
    lastDrawTime_{GetTime()} {
    
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